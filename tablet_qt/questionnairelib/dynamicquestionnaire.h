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

#pragma once
#include <functional>
#include "questionnairelib/questionnaire.h"

/*

Questionnaire in which pages are generated dynamically by the caller,
and stored in a sequence so that the user can go back.

See the Questionnaire class first.

We continue to use QVector<QuPagePtr> m_pages.
Key differences:
- BACK BUTTON: pop last page; go to new last page.
- FORWARD BUTTON:
    - available when page thinks it's complete
    - when pressed, ask the caller for the next page
    - append to m_pages and go to it
- PAGE LIST:
    - as usual
    - if you jump (backwards; jumping forwards no longer makes sense),
      we go to the page you select and drop all subsequent pages from
      m_pages; state will persist in FIELD state instead, so your answers
      should persist if you go forward again without modification.
- FINISH BUTTON:
    - needs amending as now the last page in m_pages is often not the
      last overall. See below.

Rejected options:
- each task inherits from DynamicQuestionnaire and implements something
  like getPage(n); this would cause problems for integrating into the
  need for tasks to inherit from DatabaseObject as well (given that
  multiple inheritance for QObject is a no-no). So, we hook to the task
  instead.
- We don't implement multiple things like
  getNextQuestionNumber(int current_qnum) or getPageTitle(int qnum).

Instead we just ask for a callback function like
        QuPagePtr getNextPage(int current_qnum)
If this returns nullptr, we stop.

The tricky bit is that a given page might lead to:
    - end of questionnaire, show Finish
    - next page available, show Next
    - incomplete, need more info, don't show Next or Finish, show warning
dynamically, depending on the current state.

The difference between "warning" and the others can be accomplished as we
do now, using fieldref "mandatory" flags. So we only really need to think
about the difference between "another page to come" and "we're at the end",
which may differ depending on state.

One option here is to use a single function, getNextPage(), and just use
this. (We can override Questionnaire::morePagesToGo() to call it, and test
for nullptr.) The downside to this is that we might end up creating lots of
unnecessary pages, which might be expensive. The alternative is to call
another callback like

        bool morePagesToGo(int current_qnum);

and then a lazy/high-speed task can implement it as

        bool morePagesToGo(int current_qnum)
        {
            return getNextPage(current_qnum + 1) != nullptr;
        }

whereas a more complex task can optimize.

*/


class DynamicQuestionnaire : public Questionnaire
{
    Q_OBJECT

    using MakePageFn = std::function<QuPagePtr(int)>;
    // ... function taking one int parameter (the zero-based page number to
    //     make) and returning a QuPagePtr

    using MorePagesToGoFn = std::function<bool(int)>;
    // ... function taking one int parameter (the zero-based page number we're
    //     on now) and returning a bool

public:
    DynamicQuestionnaire(CamcopsApp& app,
                         const MakePageFn& make_page_fn,
                         const MorePagesToGoFn& more_pages_to_go_fn);

    // ------------------------------------------------------------------------
    // Override in order to block functionality:
    // ------------------------------------------------------------------------

    // ... we don't add pages, as we generate them dynamically
    virtual void addPage(const QuPagePtr& page) override;  // crashes if called

    // ... we don't delete pages either
    virtual void deletePage(int index) override;  // crashes if called

    // ------------------------------------------------------------------------
    // Behave differently:
    // ------------------------------------------------------------------------

    // Override to say "we are dynamic".
    virtual bool isDynamic() const override;

    // goToPage() jumps in the same way, but then deletes pages after the
    // current one
    virtual void goToPage(int index, bool allow_refresh = false) override;

    // If we're at the end, stop; otherwise, fetch the next dynamic page and
    // display it
    virtual void processNextClicked() override;

protected:
    // Are there more pages to go?
    virtual bool morePagesToGo() const override;

    // Adds the first page. Called by Questionnaire::build().
    void addFirstDynamicPage() override;

    // Add all pages that the current state will allow us to progress to.
    // (Typically used when editing a previous questionnaire and calling up
    // the page jump list.)
    void addAllAccessibleDynamicPages() override;

    // ------------------------------------------------------------------------
    // New functionality:
    // ------------------------------------------------------------------------

    // Chop off all pages beyond the current one
    void trimFromCurrentPositionOnwards();

    // "Does the specified page allow us to progress?"
    bool mayProgress(QuPage* page) const;

protected:

    // User-supplied function to make a page dynamically
    MakePageFn m_make_page_fn;

    // User-supplied function: are there more pages to come?
    MorePagesToGoFn m_more_pages_to_go_fn;
};
