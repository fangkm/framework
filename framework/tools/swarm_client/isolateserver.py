#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Archives a set of files to a server."""

__version__ = '0.1'

import binascii
import cStringIO
import functools
import hashlib
import itertools
import logging
import os
import sys
import time
import urllib
import zlib

from third_party import colorama
from third_party.depot_tools import fix_encoding
from third_party.depot_tools import subcommand

import run_isolated

from utils import net
from utils import threading_utils
from utils import tools


# Default server.
# TODO(maruel): Chromium-specific.
ISOLATE_SERVER = 'https://isolateserver-dev.appspot.com/'


# The minimum size of files to upload directly to the blobstore.
MIN_SIZE_FOR_DIRECT_BLOBSTORE = 20 * 1024

# The number of files to check the isolate server per /contains query.
# All files are sorted by likelihood of a change in the file content
# (currently file size is used to estimate this: larger the file -> larger the
# possibility it has changed). Then first ITEMS_PER_CONTAINS_QUERIES[0] files
# are taken and send to '/contains', then next ITEMS_PER_CONTAINS_QUERIES[1],
# and so on. Numbers here is a trade-off; the more per request, the lower the
# effect of HTTP round trip latency and TCP-level chattiness. On the other hand,
# larger values cause longer lookups, increasing the initial latency to start
# uploading, which is especially an issue for large files. This value is
# optimized for the "few thousands files to look up with minimal number of large
# files missing" case.
ITEMS_PER_CONTAINS_QUERIES = [20, 20, 50, 50, 50, 100]

# A list of already compressed extension types that should not receive any
# compression before being uploaded.
ALREADY_COMPRESSED_TYPES = [
    '7z', 'avi', 'cur', 'gif', 'h264', 'jar', 'jpeg', 'jpg', 'pdf', 'png',
    'wav', 'zip'
]


def randomness():
  """Generates low-entropy randomness for MIME encoding.

  Exists so it can be mocked out in unit tests.
  """
  return str(time.time())


def encode_multipart_formdata(fields, files,
                              mime_mapper=lambda _: 'application/octet-stream'):
  """Encodes a Multipart form data object.

  Args:
    fields: a sequence (name, value) elements for
      regular form fields.
    files: a sequence of (name, filename, value) elements for data to be
      uploaded as files.
    mime_mapper: function to return the mime type from the filename.
  Returns:
    content_type: for httplib.HTTP instance
    body: for httplib.HTTP instance
  """
  boundary = hashlib.md5(randomness()).hexdigest()
  body_list = []
  for (key, value) in fields:
    if isinstance(key, unicode):
      value = key.encode('utf-8')
    if isinstance(value, unicode):
      value = value.encode('utf-8')
    body_list.append('--' + boundary)
    body_list.append('Content-Disposition: form-data; name="%s"' % key)
    body_list.append('')
    body_list.append(value)
    body_list.append('--' + boundary)
    body_list.append('')
  for (key, filename, value) in files:
    if isinstance(key, unicode):
      value = key.encode('utf-8')
    if isinstance(filename, unicode):
      value = filename.encode('utf-8')
    if isinstance(value, unicode):
      value = value.encode('utf-8')
    body_list.append('--' + boundary)
    body_list.append('Content-Disposition: form-data; name="%s"; '
                     'filename="%s"' % (key, filename))
    body_list.append('Content-Type: %s' % mime_mapper(filename))
    body_list.append('')
    body_list.append(value)
    body_list.append('--' + boundary)
    body_list.append('')
  if body_list:
    body_list[-2] += '--'
  body = '\r\n'.join(body_list)
  content_type = 'multipart/form-data; boundary=%s' % boundary
  return content_type, body


def sha1_file(filepath):
  """Calculates the SHA-1 of a file without reading it all in memory at once."""
  digest = hashlib.sha1()
  with open(filepath, 'rb') as f:
    while True:
      # Read in 1mb chunks.
      chunk = f.read(1024*1024)
      if not chunk:
        break
      digest.update(chunk)
  return digest.hexdigest()


def url_read(url, **kwargs):
  result = net.url_read(url, **kwargs)
  if result is None:
    # If we get no response from the server, assume it is down and raise an
    # exception.
    raise run_isolated.MappingError('Unable to connect to server %s' % url)
  return result


def upload_hash_content_to_blobstore(
    generate_upload_url, data, hash_key, content):
  """Uploads the given hash contents directly to the blobstore via a generated
  url.

  Arguments:
    generate_upload_url: The url to get the new upload url from.
    data: extra POST data.
    hash_key: sha1 of the uncompressed version of content.
    content: The contents to upload. Must fit in memory for now.
  """
  logging.debug('Generating url to directly upload file to blobstore')
  assert isinstance(hash_key, str), hash_key
  assert isinstance(content, str), (hash_key, content)
  # TODO(maruel): Support large files. This would require streaming support.
  content_type, body = encode_multipart_formdata(
      data, [('content', hash_key, content)])
  for attempt in xrange(net.URL_OPEN_MAX_ATTEMPTS):
    # Retry HTTP 50x here.
    upload_url = net.url_read(generate_upload_url, data=data)
    if not upload_url:
      raise run_isolated.MappingError(
          'Unable to connect to server %s' % generate_upload_url)

    # Do not retry this request on HTTP 50x. Regenerate an upload url each time
    # since uploading "consumes" the upload url.
    result = net.url_read(
        upload_url, data=body, content_type=content_type, retry_50x=False)
    if result is not None:
      return result
    if attempt != net.URL_OPEN_MAX_ATTEMPTS - 1:
      net.HttpService.sleep_before_retry(attempt, None)
  raise run_isolated.MappingError(
      'Unable to connect to server %s' % generate_upload_url)


def upload_file(base_url, namespace, content, hash_key, token):
  # TODO(maruel): Detect failures.
  hash_key = str(hash_key)
  content_url = base_url.rstrip('/') + '/content/'
  if len(content) > MIN_SIZE_FOR_DIRECT_BLOBSTORE:
    url = '%sgenerate_blobstore_url/%s/%s' % (
        content_url, namespace, hash_key)
    # token is guaranteed to be already quoted but it is unnecessary here, and
    # only here.
    data = [('token', urllib.unquote(token))]
    return upload_hash_content_to_blobstore(url, data, hash_key, content)
  else:
    url = '%sstore/%s/%s?token=%s' % (
        content_url, namespace, hash_key, token)
    return url_read(url, data=content, content_type='application/octet-stream')


class UploadRemote(run_isolated.Remote):
  def __init__(self, namespace, base_url, token):
    self.namespace = str(namespace)
    self._token = token
    super(UploadRemote, self).__init__(base_url)

  def get_file_handler(self, base_url):
    base_url = str(base_url)
    return functools.partial(
        upload_file, base_url, self.namespace, token=self._token)


def check_files_exist_on_server(query_url, queries):
  """Queries the server to see which files from this batch already exist there.

  Arguments:
    queries: The hash files to potential upload to the server.
  Returns:
    missing_files: list of files that are missing on the server.
  """
  logging.info('Checking existence of %d files...', len(queries))
  body = ''.join(
      (binascii.unhexlify(meta_data['h']) for (_, meta_data) in queries))
  assert (len(body) % 20) == 0, repr(body)

  response = url_read(
      query_url, data=body, content_type='application/octet-stream')
  if len(queries) != len(response):
    raise run_isolated.MappingError(
        'Got an incorrect number of responses from the server. Expected %d, '
        'but got %d' % (len(queries), len(response)))

  missing_files = [
    queries[i] for i, flag in enumerate(response) if flag == chr(0)
  ]
  logging.info('Queried %d files, %d cache hit',
               len(queries), len(queries) - len(missing_files))
  return missing_files


def compression_level(filename):
  """Given a filename calculates the ideal compression level to use."""
  file_ext = os.path.splitext(filename)[1].lower()
  # TODO(csharp): Profile to find what compression level works best.
  return 0 if file_ext in ALREADY_COMPRESSED_TYPES else 7


def read_and_compress(filepath, level):
  """Reads a file and returns its content gzip compressed."""
  compressor = zlib.compressobj(level)
  compressed_data = cStringIO.StringIO()
  with open(filepath, 'rb') as f:
    while True:
      chunk = f.read(run_isolated.ZIPPED_FILE_CHUNK)
      if not chunk:
        break
      compressed_data.write(compressor.compress(chunk))
  compressed_data.write(compressor.flush(zlib.Z_FINISH))
  value = compressed_data.getvalue()
  compressed_data.close()
  return value


def zip_and_trigger_upload(infile, metadata, upload_function):
  # TODO(csharp): Fix crbug.com/150823 and enable the touched logic again.
  # if not metadata['T']:
  compressed_data = read_and_compress(infile, compression_level(infile))
  priority = (
      run_isolated.Remote.HIGH if metadata.get('priority', '1') == '0'
      else run_isolated.Remote.MED)
  return upload_function(priority, compressed_data, metadata['h'], None)


def batch_files_for_check(infiles):
  """Splits list of files to check for existence on the server into batches.

  Each batch corresponds to a single 'exists?' query to the server.

  Yields:
    batches: list of batches, each batch is a list of files.
  """
  batch_count = 0
  batch_size_limit = ITEMS_PER_CONTAINS_QUERIES[0]
  next_queries = []
  items = ((k, v) for k, v in infiles.iteritems() if 's' in v)
  for relfile, metadata in sorted(items, key=lambda x: -x[1]['s']):
    next_queries.append((relfile, metadata))
    if len(next_queries) == batch_size_limit:
      yield next_queries
      next_queries = []
      batch_count += 1
      batch_size_limit = ITEMS_PER_CONTAINS_QUERIES[
          min(batch_count, len(ITEMS_PER_CONTAINS_QUERIES) - 1)]
  if next_queries:
    yield next_queries


def get_files_to_upload(contains_hash_url, infiles):
  """Yields files that are missing on the server."""
  with threading_utils.ThreadPool(1, 16, 0, prefix='get_files_to_upload') as tp:
    for files in batch_files_for_check(infiles):
      tp.add_task(0, check_files_exist_on_server, contains_hash_url, files)
    for missing_file in itertools.chain.from_iterable(tp.iter_results()):
      yield missing_file


def upload_sha1_tree(base_url, indir, infiles, namespace):
  """Uploads the given tree to the given url.

  Arguments:
    base_url:  The base url, it is assume that |base_url|/has/ can be used to
               query if an element was already uploaded, and |base_url|/store/
               can be used to upload a new element.
    indir:     Root directory the infiles are based in.
    infiles:   dict of files to upload files from |indir| to |base_url|.
    namespace: The namespace to use on the server.
  """
  logging.info('upload tree(base_url=%s, indir=%s, files=%d)' %
               (base_url, indir, len(infiles)))
  assert base_url.startswith('http'), base_url
  base_url = base_url.rstrip('/')

  # TODO(maruel): Make this request much earlier asynchronously while the files
  # are being enumerated.
  token = urllib.quote(url_read(base_url + '/content/get_token'))

  # Create a pool of workers to zip and upload any files missing from
  # the server.
  num_threads = threading_utils.num_processors()
  zipping_pool = threading_utils.ThreadPool(min(2, num_threads),
                                            num_threads, 0, 'zip')
  remote_uploader = UploadRemote(namespace, base_url, token)

  # Starts the zip and upload process for files that are missing
  # from the server.
  contains_hash_url = '%s/content/contains/%s?token=%s' % (
      base_url, namespace, token)
  uploaded = []
  for relfile, metadata in get_files_to_upload(contains_hash_url, infiles):
    infile = os.path.join(indir, relfile)
    zipping_pool.add_task(0, zip_and_trigger_upload, infile, metadata,
                          remote_uploader.add_item)
    uploaded.append((relfile, metadata))

  logging.info('Waiting for all files to finish zipping')
  zipping_pool.join()
  zipping_pool.close()
  logging.info('All files zipped.')

  logging.info('Waiting for all files to finish uploading')
  # Will raise if any exception occurred.
  remote_uploader.join()
  remote_uploader.close()
  logging.info('All files are uploaded')

  total = len(infiles)
  total_size = sum(metadata.get('s', 0) for metadata in infiles.itervalues())
  logging.info(
      'Total:      %6d, %9.1fkb',
      total,
      sum(m.get('s', 0) for m in infiles.itervalues()) / 1024.)
  cache_hit = set(infiles.iterkeys()) - set(x[0] for x in uploaded)
  cache_hit_size = sum(infiles[i].get('s', 0) for i in cache_hit)
  logging.info(
      'cache hit:  %6d, %9.1fkb, %6.2f%% files, %6.2f%% size',
      len(cache_hit),
      cache_hit_size / 1024.,
      len(cache_hit) * 100. / total,
      cache_hit_size * 100. / total_size if total_size else 0)
  cache_miss = uploaded
  cache_miss_size = sum(infiles[i[0]].get('s', 0) for i in cache_miss)
  logging.info(
      'cache miss: %6d, %9.1fkb, %6.2f%% files, %6.2f%% size',
      len(cache_miss),
      cache_miss_size / 1024.,
      len(cache_miss) * 100. / total,
      cache_miss_size * 100. / total_size if total_size else 0)
  return 0


@subcommand.usage('<file1..fileN> or - to read from stdin')
def CMDarchive(parser, args):
  """Archives data to the server."""
  options, files = parser.parse_args(args)

  if files == ['-']:
    files = sys.stdin.readlines()

  if not files:
    parser.error('Nothing to upload')
  if not options.isolate_server:
    parser.error('Nowhere to send. Please specify --isolate-server')

  # Load the necessary metadata. This is going to be rewritten eventually to be
  # more efficient.
  infiles = dict(
      (
        f,
        {
          's': os.stat(f).st_size,
          'h': sha1_file(f),
        }
      )
      for f in files)

  with tools.Profiler('Archive'):
    return upload_sha1_tree(
        base_url=options.isolate_server,
        indir=os.getcwd(),
        infiles=infiles,
        namespace=options.namespace)
  return 0


def CMDdownload(parser, args):
  """Download data from the server."""
  _options, args = parser.parse_args(args)
  parser.error('Sorry, it\'s not really supported.')
  return 0


class OptionParserIsolateServer(tools.OptionParserWithLogging):
  def __init__(self, **kwargs):
    tools.OptionParserWithLogging.__init__(self, **kwargs)
    self.add_option(
        '-I', '--isolate-server',
        default=ISOLATE_SERVER,
        metavar='URL',
        help='Isolate server where data is stored. default: %default')
    self.add_option(
        '--namespace', default='default-gzip',
        help='The namespace to use on the server.')

  def parse_args(self, *args, **kwargs):
    options, args = tools.OptionParserWithLogging.parse_args(
        self, *args, **kwargs)
    options.isolate_server = options.isolate_server.rstrip('/')
    if not options.isolate_server:
      self.error('--isolate-server is required.')
    return options, args


def main(args):
  dispatcher = subcommand.CommandDispatcher(__name__)
  try:
    return dispatcher.execute(
        OptionParserIsolateServer(version=__version__), args)
  except (
      run_isolated.MappingError,
      run_isolated.ConfigError) as e:
    sys.stderr.write('\nError: ')
    sys.stderr.write(str(e))
    sys.stderr.write('\n')
    return 1


if __name__ == '__main__':
  fix_encoding.fix_encoding()
  tools.disable_buffering()
  colorama.init()
  sys.exit(main(sys.argv[1:]))
