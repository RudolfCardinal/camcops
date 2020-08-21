/*
    Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).

    This file is part of CamCOPS.

    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.
*/

#include "dynamicquestionnaire.h"
#include <QDebug>
#include "lib/uifunc.h"


DynamicQuestionnaire::DynamicQuestionnaire(
        CamcopsApp& app,
        const MakePageFn& make_page_fn,
        const MorePagesToGoFn& more_pages_to_go_fn) :
    Questionnaire(app),
    m_make_page_fn(make_page_fn),
    m_more_pages_to_go_fn(more_pages_to_go_fn)
{
    Q_ASSERT(m_current_page_index == 0);
    // Rude to call back into our owner during construction.
    // See addFirstDynamicPage() instead.
}


bool DynamicQuestionnaire::isDynamic() const
{
    return true;
}


void DynamicQuestionnaire::addFirstDynamicPage()
{
    QuPagePtr first_page = m_make_page_fn(m_current_page_index);
    if (!first_page) {
        uifunc::stopApp("Dynamic questionnaire created but caller refuses to "
                        "supply first page");
    }
    m_pages.append(first_page);
}


void DynamicQuestionnaire::addAllAccessibleDynamicPages()
{
    // Now, it may be that there are more to come. For example, if we're
    // editing a task that has previously been completed, there may be lots
    // of pages we can traverse to. Unless we collect them now, we won't be
    // able to jump past pages (we'd just be permitted to click "Next" a lot).
    // Since what we can access depends on the read-only status, we call this
    // AFTER the client has had a chance to set the read-only status.

    trimFromCurrentPositionOnwards();
    // ... or potential for inconsistency, e.g. if we're jumping, and we've
    // made a different decision on this page.

    int page_index = nPages() - 1;  // current last page
    QuPagePtr page;
    do {
        page = m_pages[page_index];
        // Not quite the same as a standard questionnaire (see processNextClicked).
        // We don't allow progress for blocked/missing-input pages in the
        // read-only situation (or, for example, you get a ridiculous list of
        // inaccessible pages; try the CIS-R).
        bool may_progress = page && page->mayProgressIgnoringValidators();
        may_progress = may_progress && m_more_pages_to_go_fn(page_index);
        if (may_progress) {
            ++page_index;
            page = m_make_page_fn(page_index);
            if (page) {
                m_pages.append(page);
            }
        } else {
            page = nullptr;
        }
    } while (page);
}


bool DynamicQuestionnaire::morePagesToGo() const
{
    const int current_qnum = currentPageIndex();
    return m_more_pages_to_go_fn(current_qnum);
}


void DynamicQuestionnaire::addPage(const QuPagePtr& page)
{
    Q_UNUSED(page)
    uifunc::stopApp("Don't call addPage() on a DynamicQuestionnaire!");
}


void DynamicQuestionnaire::deletePage(const int index)
{
    Q_UNUSED(index)
    uifunc::stopApp("Don't call deletePage() on a DynamicQuestionnaire!");
}


void DynamicQuestionnaire::goToPage(const int index, const bool allow_refresh)
{
    if (index < 0 || index >= nPages()) {
        qWarning() << Q_FUNC_INFO << "Invalid index:" << index;
        return;
    }
    if (index == m_current_page_index && !allow_refresh) {
        qDebug() << "Page" << index <<
                    "(zero-based index) already selected";
        return;
    }
    pageClosing();
    m_current_page_index = index;

    // Now the bit that's different for DynamicQuestionnaire:
    trimFromCurrentPositionOnwards();

    // Back to Questionnaire behaviour:
    build();
}


void DynamicQuestionnaire::trimFromCurrentPositionOnwards()
{
    // Chop off all pages beyond the current one
    while (m_pages.length() > m_current_page_index + 1) {
        m_pages.removeLast();
    }
}


void DynamicQuestionnaire::processNextClicked()
{
    // As per Questionnaire:
    QuPage* page = currentPagePtr();
    // Not quite the same as a standard questionnaire (see processNextClicked).
    // We don't allow progress for blocked/missing-input pages in the
    // read-only situation (or, for example, you get a ridiculous list of
    // inaccessible pages; try the CIS-R).
    if (!page || !page->mayProgressIgnoringValidators() || !page->validate()) {
        return;
    }

    // Different:
    // not now: allowing jump-ahead // Q_ASSERT(m_current_page_index == m_pages.length() - 1);
    trimFromCurrentPositionOnwards();
    const int next_qnum = m_current_page_index + 1;
    QuPagePtr new_dynamic_page = m_make_page_fn(next_qnum);
    if (!new_dynamic_page) {
        qWarning()
                << Q_FUNC_INFO
                << "Miscalculation: we have offered a Next button but the "
                   "task wants to finish, so we should have offered a Finish "
                   "button; this implies the task has got its "
                   "'more_pages_to_go_fn' function wrong";
        doFinish();
        return;
    }
    m_pages.append(new_dynamic_page);
    goToPage(next_qnum);
}
