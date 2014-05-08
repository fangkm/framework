/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AccessibilityTable_h
#define AccessibilityTable_h

#include "core/accessibility/AccessibilityRenderObject.h"
#include "wtf/Forward.h"

namespace WebCore {

class AccessibilityTableCell;
class RenderTableSection;

class AccessibilityTable : public AccessibilityRenderObject {

protected:
    explicit AccessibilityTable(RenderObject*);
public:
    static PassRefPtr<AccessibilityTable> create(RenderObject*);
    virtual ~AccessibilityTable();

    virtual void init() OVERRIDE;

    virtual bool isAccessibilityTable() const OVERRIDE;
    virtual bool isDataTable() const OVERRIDE;

    virtual AccessibilityRole roleValue() const OVERRIDE;

    virtual void addChildren() OVERRIDE;
    virtual void clearChildren() OVERRIDE;

    // To be overridden by AccessibilityARIAGrid.
    virtual bool isAriaTable() const { return false; }
    virtual bool supportsSelectedRows() { return false; }

    AccessibilityChildrenVector& columns();
    AccessibilityChildrenVector& rows();

    unsigned columnCount();
    unsigned rowCount();
    virtual int tableLevel() const OVERRIDE;

    virtual String title() const OVERRIDE;

    // all the cells in the table
    void cells(AccessibilityChildrenVector&);
    AccessibilityTableCell* cellForColumnAndRow(unsigned column, unsigned row);

    void columnHeaders(AccessibilityChildrenVector&);

    // an object that contains, as children, all the objects that act as headers
    AccessibilityObject* headerContainer();

protected:
    AccessibilityChildrenVector m_rows;
    AccessibilityChildrenVector m_columns;

    RefPtr<AccessibilityObject> m_headerContainer;
    bool m_isAccessibilityTable;

    bool hasARIARole() const;
    virtual bool isTableExposableThroughAccessibility() const;
    virtual bool computeAccessibilityIsIgnored() const OVERRIDE;
};

inline AccessibilityTable* toAccessibilityTable(AccessibilityObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isAccessibilityTable());
    return static_cast<AccessibilityTable*>(object);
}

} // namespace WebCore

#endif // AccessibilityTable_h
