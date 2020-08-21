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

// #define QUESTIONAIRE_USE_HFW_LAYOUT  // bad; contained scroll area gets too short

#include <initializer_list>
#include <QList>
#include <QPointer>
#include <QSharedPointer>
#include "common/aliases_camcops.h"
#include "common/uiconst.h"  // for FontSize
#include "layouts/layouts.h"
#include "questionnairelib/qupage.h"
#include "widgets/openablewidget.h"

class CamcopsApp;
class QuestionnaireHeader;
class QVBoxLayout;
class QWidget;


class Questionnaire : public OpenableWidget
{
    // Master class controlling a questionnaire.

    Q_OBJECT
public:
    // ========================================================================
    // Constructors
    // ========================================================================
    Questionnaire(CamcopsApp& app);
    Questionnaire(CamcopsApp& app, const QVector<QuPagePtr>& pages);
    Questionnaire(CamcopsApp& app, std::initializer_list<QuPagePtr> pages);
    Questionnaire(CamcopsApp& app, const QVector<QuPage*> pages);  // takes ownership
    Questionnaire(CamcopsApp& app, std::initializer_list<QuPage*> pages);  // takes ownership

    // ========================================================================
    // Information about the questionnaire
    // ========================================================================

    // Is this questionnaire in read-only mode?
    bool readOnly() const;

    // What's the zero-based index of the current page?
    int currentPageIndex() const;

    // What's the one-based index of the current page (for display purposes)?
    int currentPageNumOneBased() const;

    // How many pages does the questionnaire have?
    // (For *dynamic* questionnaires: this includes from the first to the
    // current, typically, or from the first to the last that is accessible;
    // it's not reliable as an overall page count, as that can vary depending
    // on the user's answers.)
    int nPages() const;

    // Is this a dynamic questionnaire? See DynamicQuestionnaire.
    virtual bool isDynamic() const;

    // ========================================================================
    // Build widgets when the Questionnaire is displayed
    // ========================================================================

    // Build the master widgets. Ensure we are displaying a page.
    virtual void build() override;

    // ========================================================================
    // Set attributes about the questionnaire
    // ========================================================================

    // Sets the master type for the questionnaire -- e.g. Patient, Clinician.
    // This allows pages to inherit their type from the questionnaire.
    // (Pages can also override this on a per-page basis.)
    // The type sets the page's background colour, so the user gets a hint as
    // to who's meant to be answering the questions.
    void setType(QuPage::PageType type);

    // Sets the read-only status.
    void setReadOnly(bool read_only = true);

    // Should the questionnaire's QuestionnaireHeader offer a "jump to page"
    // button ?
    void setJumpAllowed(bool jump_allowed = true);

    // For "chain multiple questionnaires together" function via TaskChain
    // (i.e. do one task, then do another). Currently, this only
    // affects the "end" button's appearance (fast forward versus stop).
    void setWithinChain(bool within_chain = true);

    // Sets the icon for the "finish" button (e.g. a tick for config editing
    // questionnaires; a stop icon for task questionnaires).
    void setFinishButtonIcon(const QString& base_filename);

    // Sets the "finish" icon to a tick mark (for config editing
    // questionnaires).
    void setFinishButtonIconToTick();

    // ========================================================================
    // Add pages
    // ========================================================================

    // These functions add a new page to the end of the questionnaire.
    virtual void addPage(const QuPagePtr& page);
    virtual void addPage(QuPage* page);  // takes ownership

    // ========================================================================
    // Get page information
    // ========================================================================

    // Return a pointer to the page currently being displayed.
    QuPage* currentPagePtr() const;

    // Return a pointer to the specified page.
    QuPage* pagePtr(int index) const;

    // Return pointers to pages matching our criteria:
    // - current_page_only: restrict to the currently displayed page
    // - page_tag: restrict to pages having the specified tag (see QuPage)
    QVector<QuPage*> getPages(bool current_page_only,
                              const QString& page_tag = "");

    // ========================================================================
    // Alter pages
    // ========================================================================

    // Sets the "skip" flag for a particular page -- either by page index
    // (zero-based)  or by tag (applying the change to all pages having the
    // specified tag).
    // If a page is marked as "skip", it is not shown, and it does not block
    // the appearance of subsequent pages.
    // This is a simple way of implementing conditional logic; e.g. "if the
    // user reports no sleep problems, skip questions about sleep problems".
    // If reset_buttons == true, also calls resetButtons().
    void setPageSkip(int page, bool skip, bool reset_buttons = true);
    void setPageSkip(const QString& page_tag, bool skip,
                     bool reset_buttons = true);

    // Deletes a page by its (zero-based) index.
    virtual void deletePage(int index);

    // Moves the specified page to a position one earlier in the page list.
    void movePageBackwards(int index);

    // Moves the specified page to a position one later in the page list.
    void movePageForwards(int index);

    // ========================================================================
    // Get element information
    // ========================================================================

    // Return all elements having the specified tag (see QuElement).
    // If current_page_only, restrict to elements from the current page.
    QVector<QuElement*> getElementsByTag(const QString& tag,
                                         bool current_page_only = true,
                                         const QString& page_tag = "");

    // Returns the first element having the specified tag.
    // Otherwise as per getElementsByTag().
    QuElement* getFirstElementByTag(const QString& tag,
                                    bool current_page_only = true,
                                    const QString& page_tag = "");

    // ========================================================================
    // Alter elements
    // ========================================================================

    // Find elements having the specified tag -- see getElementsByTag() -- and
    // set their visibility status (visible or invisible).
    void setVisibleByTag(const QString& tag, bool visible,
                         bool current_page_only = true,
                         const QString& page_tag = "");

    // ========================================================================
    // Page control
    // ========================================================================

    // Refresh the current page. Some pages may choose to do this if their
    // widgets change substantially (see e.g. PhotoSequence,
    // DiagnosisTaskBase).
    void refreshCurrentPage();

    // Jump to a specific page. If allow_refresh is false, then if you jump to
    // the page you're already on, nothing happens (but if it's true, the
    // current page is refreshed).
    virtual void goToPage(int index, bool allow_refresh = false);

    // ========================================================================
    // Advanced control
    // ========================================================================

    // This is used to open major/complex editing widgets, such as the
    // camera control aspects used by QuPhoto, and the diagnostic code
    // selection widget of QuDiagnosticCode. It opens the specified widget in
    // a new window in the CamCOPS window stack.
    void openSubWidget(OpenableWidget* widget);

    // ========================================================================
    // Utility functions
    // ========================================================================

    // Returns the CamcopsApp object.
    CamcopsApp& app() const;

    // Converts a font size type (e.g. "big", "normal") into a specific font
    // size, via the CamcopsApp's current font zoom setting.
    int fontSizePt(uiconst::FontSize fontsize) const;

    // Returns CSS for the questionnaire, processed via the CamcopsApp's
    // current font zoom setting.
    QString getSubstitutedCss(const QString& filename) const;

    // Dumps the questionnaire's widget layout to the debugging stream.
    void debugLayout();

public slots:

    // "Calculate whether the user is allowed to move to the previous page,
    // to the next page, or finish. Ask our QuestionnaireHeader to refresh its
    // buttons accordinly."
    void resetButtons();

signals:

    // "We have started editing."
    void editStarted();

    // "We have finished editing -- either because the user finished or because
    // they aborted." Emitted just before cancelled() or completed().
    // Not emitted when read-only questionnaires finish.
    void editFinished(bool aborted);

    // "A page is about to open." Used for a specific hook to allow the
    // SettingsMenu to detect font sizes change.
    void pageAboutToOpen();  // about to display page

    // "User has cancelled." Emitted upon failure/cancel, just before
    // OpenableWidget::finished() is emitted.
    void cancelled();

    // "User has completed." Emitted upon success/OK, just before
    // OpenableWidget::finished() is emitted.
    void completed();

protected:
    // Called by build(); overridden in DynamicQuestionnaire.
    virtual void addFirstDynamicPage();

    // Are there more (non-skip) pages after the current one?
    virtual bool morePagesToGo() const;

    // Called by jumpClicked(); overridden in DynamicQuestionnaire.
    virtual void addAllAccessibleDynamicPages();

    // If "finish" is clicked on the last page, we end up here. Emits
    // appropriate signals.
    void doFinish();

    // If the user cancels editing, we end up here. Emits  appropriate signals.
    void doCancel();

    // When we change page, we tell the current page that we're closing via
    // this function, which calls QuPage::closing(), before we move to the new
    // page. In turn, the page tells its elements that they're closing. They
    // may want to act, e.g. to stop audio playback if we leave a page with an
    // audio player that's currently playing.
    void pageClosing();

    // Qt signal handling: e.g. emits editStarted() when the questionnaire is
    // shown.
    bool event(QEvent* e) override;

    // Qt signal handling: e.g. translates the Escape key to "cancel".
    void keyPressEvent(QKeyEvent* event) override;

    // User has clicked "next". If valid, advance to the next page.
    virtual void processNextClicked();

    // The stylesheet we will apply.
    QString questionnaireStylesheet() const;

protected slots:

    // "User has clicked cancel." Check they mean it, etc.
    void cancelClicked();

    // "User has clicked jump-to-page." Offer a page menu.
    void jumpClicked();

    // "User has clicked 'previous page'."
    void previousClicked();

    // "User has clicked 'next page'."
    void nextClicked();

    // "User has clicked 'finish'."
    void finishClicked();

protected:
    CamcopsApp& m_app;  // our app
    QVector<QuPagePtr> m_pages;  // our pages
    QuPage::PageType m_type;  // our type, e.g. patient/clinician (pages may inherit or override)
    bool m_read_only;  // are we in read-only mode?
    bool m_jump_allowed;  // is the user allowed to jump to a page?
    bool m_within_chain;  // set setWithinChain()

    bool m_built;
#ifdef QUESTIONAIRE_USE_HFW_LAYOUT
    QPointer<VBoxLayout> m_outer_layout;
    QPointer<VBoxLayout> m_mainlayout;
#else
    QPointer<QVBoxLayout> m_outer_layout;  // see layout described in build()
    QPointer<QVBoxLayout> m_mainlayout;  // see layout described in build()
#endif
    QPointer<QWidget> m_background_widget;  // see layout described in build()
    QPointer<QuestionnaireHeader> m_p_header;  // see layout described in build()
    int m_current_page_index;  // zero-based index of the current page
    QString m_finish_button_icon_base_filename;  // see setFinishButtonIcon()
};
