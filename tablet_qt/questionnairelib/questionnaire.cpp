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

// #define OFFER_LAYOUT_DEBUG_BUTTON
// #define DEBUG_PAGE_LAYOUT_ON_OPEN
// #define DEBUG_REPORT_OPEN_SUBWIDGET
// #define DISABLE_ZOOMABLE_WIDGET

#include "questionnaire.h"
#include <functional>
#include <QDebug>
#include <QKeyEvent>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include "core/camcopsapp.h"
#include "common/cssconst.h"
#include "dialogs/pagepickerdialog.h"
#include "lib/filefunc.h"
#include "lib/layoutdumper.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"
#ifdef DEBUG_PAGE_LAYOUT_ON_OPEN
#include "qobjects/showwatcher.h"
#endif
#include "questionnairelib/pagepickeritem.h"
#include "questionnairelib/questionnaireheader.h"
#include "tasklib/task.h"
#include "widgets/labelwordwrapwide.h"
#include "widgets/verticalscrollarea.h"
#include "widgets/zoomablewidget.h"


Questionnaire::Questionnaire(CamcopsApp& app) :
    Questionnaire(app, QVector<QuPagePtr>())  // delegating constructor
{
}


Questionnaire::Questionnaire(CamcopsApp& app,
                             const QVector<QuPagePtr>& pages) :
    m_app(app),
    m_pages(pages),
    m_type(QuPage::PageType::Patient),
    m_read_only(false),
    m_jump_allowed(true),
    m_within_chain(false),
    m_built(false),
#ifdef QUESTIONAIRE_USE_HFW_LAYOUT
    m_outer_layout(new VBoxLayout()),
#else
    m_outer_layout(new QVBoxLayout()),
#endif
    m_current_page_index(0)  // starting page
{
    setStyleSheet(questionnaireStylesheet());

    setLayout(m_outer_layout);
    // You can't reset the outer layout for a widget, I think. You get:
    //      QWidget::setLayout: Attempting to set QLayout "" on Questionnaire
    //      "", which already has a layout

    setEscapeKeyCanAbort(false);
}


Questionnaire::Questionnaire(CamcopsApp& app,
                             std::initializer_list<QuPagePtr> pages) :
    Questionnaire(app, QVector<QuPagePtr>(pages))  // delegating constructor
{
}


Questionnaire::Questionnaire(CamcopsApp& app,
                             const QVector<QuPage*> pages) :
    Questionnaire(app, QVector<QuPagePtr>())  // delegating constructor
{
    for (auto p : pages) {
        addPage(p);
    }
}


Questionnaire::Questionnaire(CamcopsApp& app,
                             std::initializer_list<QuPage*> pages) :
    Questionnaire(app, QVector<QuPagePtr>())  // delegating constructor
{
    for (auto p : pages) {
        addPage(p);
    }
}


void Questionnaire::setType(const QuPage::PageType type)
{
    if (type == QuPage::PageType::Inherit) {
        qWarning() << Q_FUNC_INFO << "Can only set PageType::Inherit on Page, "
                                     "not Questionnaire";
    } else {
        m_type = type;
    }
}


void Questionnaire::addPage(const QuPagePtr& page)
{
    m_pages.append(page);
}


void Questionnaire::addPage(QuPage* page)
{
    // If you add a nullptr, it will be ignored.
    if (page) {
        addPage(QuPagePtr(page));  // takes ownership
    }
}


void Questionnaire::setReadOnly(const bool read_only)
{
    m_read_only = read_only;
}


void Questionnaire::setJumpAllowed(const bool jump_allowed)
{
    m_jump_allowed = jump_allowed;
}


void Questionnaire::setWithinChain(const bool within_chain)
{
    m_within_chain = within_chain;
}


void Questionnaire::build()
{
    // ========================================================================
    // Clean up any old page widgets
    // ========================================================================
    if (m_p_header) {
        m_p_header->deleteLater();  // later, in case it's currently calling us
    }
    if (m_mainlayout) {
        m_mainlayout->deleteLater();
    }
    if (m_background_widget) {
        m_background_widget->deleteLater();
    }

    // ========================================================================
    // Create new
    // ========================================================================
    // OVERVIEW OF WIDGET/LAYOUT STRUCTURE:
    //
    // W this = OpenableWidget (inherits from QWidget)
    //   L m_outer_layout = VBoxLayout
    //     W m_background_widget = QWidget
    //       L m_mainlayout = VBoxLayout
    //         W m_p_header = QuestionnaireHeader
    //
    // Then, one of the following:
    //
    //         W scroll = VerticalScrollArea
    //           W pagewidget = BaseWidget (QWidget)
    //         . stretch
    //        ---
    //         W zw = ZoomableWidget
    //           V ZoomableGraphicsView
    //           S QGraphicsScene
    //             P QGraphicsProxyWidget
    //               W pagewidget = BaseWidget (QWidget)
    //        ---
    //         W pagewidget = BaseWidget (QWidget)
    //         . stretch

    // For dynamic questionnaires:
    if (m_pages.empty()) {
        addFirstDynamicPage();
    }

    // Get page
    if (m_current_page_index < 0 ||
            m_current_page_index >= m_pages.size()) {
        // Duff page!
        qWarning() << Q_FUNC_INFO << "Bad page number:"
                   << m_current_page_index;
        uifunc::stopApp("BUG! Bad page number in Questionnaire::build");
    }
    QuPage* page = currentPagePtr();
    if (!page) {
        uifunc::stopApp("BUG! Null page pointer in Questionnaire::build");
    }

    // In case we're building on the fly...
    page->build();

    // Page type and CSS name for background
    QuPage::PageType page_type = page->type();
    if (page_type == QuPage::PageType::Inherit) {
        page_type = m_type;
    }
    QString background_css_name;
    switch (page_type) {
    case QuPage::PageType::Patient:
    case QuPage::PageType::ClinicianWithPatient:
    default:
        background_css_name = cssconst::QUESTIONNAIRE_BACKGROUND_PATIENT;
        break;
    case QuPage::PageType::Clinician:
        background_css_name = cssconst::QUESTIONNAIRE_BACKGROUND_CLINICIAN;
        break;
    case QuPage::PageType::Config:
        background_css_name = cssconst::QUESTIONNAIRE_BACKGROUND_CONFIG;
        break;
    }

    // Header
    QString header_css_name;
    if (page_type == QuPage::PageType::ClinicianWithPatient) {
        // Header has "clinician" style; main page has "patient" style
        header_css_name = cssconst::QUESTIONNAIRE_BACKGROUND_CLINICIAN;
    } else {
        header_css_name = background_css_name;
    }
#ifdef OFFER_LAYOUT_DEBUG_BUTTON
    const bool offer_debug_layout = true;
#else
    const bool offer_debug_layout = false;
#endif
    const bool offer_page_jump = m_jump_allowed && (isDynamic() || nPages() > 1);
    m_p_header = new QuestionnaireHeader(
        this, page->title(),
        m_read_only, offer_page_jump, m_within_chain,
        header_css_name, offer_debug_layout);
    if (!m_finish_button_icon_base_filename.isEmpty()) {
        m_p_header->setFinishButtonIcon(m_finish_button_icon_base_filename);
    }
    connect(m_p_header, &QuestionnaireHeader::cancelClicked,
            this, &Questionnaire::cancelClicked);
    connect(m_p_header, &QuestionnaireHeader::jumpClicked,
            this, &Questionnaire::jumpClicked);
    connect(m_p_header, &QuestionnaireHeader::previousClicked,
            this, &Questionnaire::previousClicked);
    connect(m_p_header, &QuestionnaireHeader::nextClicked,
            this, &Questionnaire::nextClicked);
    connect(m_p_header, &QuestionnaireHeader::finishClicked,
            this, &Questionnaire::finishClicked);
    connect(m_p_header, &QuestionnaireHeader::debugLayout,
            this, &Questionnaire::debugLayout);

    // Content
    QWidget* pagewidget = page->widget(this);  // adds the content
#ifdef DEBUG_PAGE_LAYOUT_ON_OPEN
    auto showwatcher = new ShowWatcher(pagewidget, true);
    Q_UNUSED(showwatcher)
#endif
    connect(page, &QuPage::elementValueChanged,
            this, &Questionnaire::resetButtons,
            Qt::UniqueConnection);

    // Main layout: header and scrollable content
#ifdef QUESTIONAIRE_USE_HFW_LAYOUT
    m_mainlayout = new VBoxLayout();
#else
    m_mainlayout = new QVBoxLayout();
#endif
    m_mainlayout->setContentsMargins(uiconst::NO_MARGINS);
    m_mainlayout->addWidget(m_p_header);

    if (page->allowsScroll()) {
        // Page sits inside a vertically scrolling area.
        // The QScrollArea (a) makes text word wrap, by setting a horizontal
        // size limit (I presume), and (b) deals with the vertical. But it
        // doesn't get the horizontal widths right. So we use a substitute.
        auto scroll = new VerticalScrollArea();
        scroll->setObjectName(background_css_name);
        scroll->setWidget(pagewidget);
        m_mainlayout->addWidget(scroll);
        // In case the questionnaire is vertically short:
        m_mainlayout->addStretch();
#ifndef DISABLE_ZOOMABLE_WIDGET
    } else if (page->isZoomable()) {
        // Page doesn't scroll but if the screen is small, the page contents
        // is zoomed out (shrunk) so it's all visible.
        pagewidget->setStyleSheet(questionnaireStylesheet());
        auto zw = new ZoomableWidget(pagewidget);
        m_mainlayout->addWidget(zw);
#endif
    } else {
        // Page as plain widget.
        m_mainlayout->addWidget(pagewidget);
        // In case the questionnaire is vertically short:
        m_mainlayout->addStretch();
    }

    // Background
    m_background_widget = new QWidget();
    /*
#ifdef QUESTIONAIRE_USE_HFW_LAYOUT
    setSizePolicy(sizehelpers::expandingExpandingHFWPolicy());
    m_background_widget->setSizePolicy(sizehelpers::expandingExpandingHFWPolicy());
#else
    m_background_widget->setSizePolicy(QSizePolicy::Expanding,
                                       QSizePolicy::Expanding);
#endif
    */
    setSizePolicy(sizehelpers::expandingExpandingHFWPolicy());
    m_background_widget->setSizePolicy(QSizePolicy::Expanding,
                                       QSizePolicy::Expanding);

    m_background_widget->setObjectName(background_css_name);
    m_background_widget->setLayout(m_mainlayout);

    // Surrounding stuff:
    m_outer_layout->addWidget(m_background_widget);
    m_outer_layout->setContentsMargins(uiconst::NO_MARGINS);

    // Finishing up
    m_built = true;

    resetButtons();

    emit pageAboutToOpen();
}


bool Questionnaire::event(QEvent* e)
{
    const bool result = OpenableWidget::event(e);
    if (!m_read_only && e->type() == QEvent::Show) {
        emit editStarted();
    }
    return result;
}


bool Questionnaire::morePagesToGo() const
{
    const int lastpage = nPages() - 1;
    for (int i = m_current_page_index; i < lastpage; ++i) {
        if (m_pages.at(i)->skip()) {
            continue;
        }
        return true;
    }
    return false;
}


void Questionnaire::resetButtons()
{
    QuPage* page = currentPagePtr();
    if (!page || !m_p_header) {
        return;
    }
    const bool allow_progression =
            readOnly() || page->mayProgressIgnoringValidators();
    // Optimization: calculate on_last_page only if necessary
    const bool on_last_page = allow_progression && !morePagesToGo();
    m_p_header->setButtons(
        m_current_page_index > 0,  // previous
        !on_last_page && allow_progression,  // next
        on_last_page && allow_progression  // finish
    );
}


int Questionnaire::currentPageIndex() const
{
    return m_current_page_index;
}


int Questionnaire::currentPageNumOneBased() const
{
    return m_current_page_index + 1;
}


int Questionnaire::nPages() const
{
    return m_pages.size();
}


bool Questionnaire::isDynamic() const
{
    return false;
}


QuPage* Questionnaire::currentPagePtr() const
{
    return pagePtr(m_current_page_index);
}


QuPage* Questionnaire::pagePtr(const int index) const
{
    if (index < 0 || index >= m_pages.size()) {
        return nullptr;
    }
    return m_pages.at(index).data();
}


bool Questionnaire::readOnly() const
{
    return m_read_only;
}


int Questionnaire::fontSizePt(const uiconst::FontSize fontsize) const
{
    return m_app.fontSizePt(fontsize);
}


void Questionnaire::keyPressEvent(QKeyEvent* event)
{
    if (!event) {
        return;
    }
    if (event->key() == Qt::Key_Escape && event->type() == QEvent::KeyPress) {
        // Escape key pressed
        cancelClicked();
    }
}


void Questionnaire::cancelClicked()
{
    if (m_read_only) {
        doCancel();
        return;
    }
    QMessageBox msgbox(this);
    msgbox.setIcon(QMessageBox::Question);
    msgbox.setWindowTitle(tr("Abort"));
    msgbox.setText(tr("Abort this questionnaire?"));
    QAbstractButton* yes = msgbox.addButton(tr("Yes, abort"),
                                            QMessageBox::YesRole);
    msgbox.addButton(tr("No, go back"), QMessageBox::NoRole);
    msgbox.exec();
    if (msgbox.clickedButton() == yes) {
        doCancel();
    }
}


void Questionnaire::addFirstDynamicPage()
{
    // only here to be overridden in DynamicQuestionnaire
    // ... but here so it can be called by build().
}


void Questionnaire::addAllAccessibleDynamicPages()
{
    // only here to be overridden in DynamicQuestionnaire
    // ... but here so it can be called by jumpClicked().
}


void Questionnaire::jumpClicked()
{
    // - In read-only mode, we can jump to any page.
    // - In editing mode, we can jump as far as the last page that isn't
    //   incomplete.
    // - We skip skipped pages in either mode.
    if (!m_read_only) {
        QuPage* current_page = currentPagePtr();
        if (current_page && !current_page->validate()) {
            return;
        }
    }
    addAllAccessibleDynamicPages();
    QVector<PagePickerItem> pageitems;
    bool blocked = false;
    for (int i = 0; i < m_pages.size(); ++i) {
        QuPagePtr page = m_pages.at(i);
        if (page->skip()) {
            continue;
            // Skipped pages don't block subsequent ones, either.
        }
        const QString text = page->indexTitle();
        const bool missing_input = !page->mayProgressIgnoringValidators();
        PagePickerItem::PagePickerItemType type = blocked
            ? PagePickerItem::PagePickerItemType::BlockedByPrevious
            : (missing_input ? PagePickerItem::PagePickerItemType::IncompleteSelectable
                             : PagePickerItem::PagePickerItemType::CompleteSelectable);
        if (!m_read_only && missing_input) {
            blocked = true;
        }
        PagePickerItem item(text, i, type);
        pageitems.append(item);
    }
    PagePickerDialog dlg(this, pageitems, tr("Choose page"));
    int new_page_zero_based;
    if (dlg.choose(&new_page_zero_based) != QDialog::Accepted) {
        return;  // user pressed cancel, or some such
    }
    goToPage(new_page_zero_based);
}


void Questionnaire::previousClicked()
{
    for (int i = m_current_page_index - 1; i >= 0; --i) {
        if (m_pages.at(i)->skip()) {
            continue;
        }
        goToPage(i);
        return;
    }
}


void Questionnaire::nextClicked()
{
    // We separate the signal receiver from the "doing things" function so
    // we can make processNextClicked() virtual for DynamicQuestionnaire (and
    // not worry about whether we connect() to the base class or derived class
    // receiver; we always connect() to the base class).
    processNextClicked();
}


void Questionnaire::processNextClicked()
{
    QuPage* page = currentPagePtr();
    if (!page) {
        return;
    }
    if (!readOnly()) {
        if (!page->mayProgressIgnoringValidators() || !page->validate()) {
            // Can't progress
            return;
        }
    }
    const int npages = nPages();
    for (int i = m_current_page_index + 1; i < npages; ++i) {
        if (m_pages.at(i)->skip()) {
            continue;
        }
        goToPage(i);
        return;
    }
}


void Questionnaire::refreshCurrentPage()
{
    goToPage(m_current_page_index, true);
}


void Questionnaire::deletePage(const int index)
{
    if (nPages() <= 1) {
        qWarning() << Q_FUNC_INFO << "Can't delete the only remaining page!";
        return;
    }

    // Step 1: if we're on the page being deleted, move cleanly to another
    //         page.
    // Step 2: delete the page (now invisible).

    const bool deleting_current = index == m_current_page_index;
    const bool deleting_earlier = index < m_current_page_index;
    bool deleting_current_and_moving_forward = false;
    if (deleting_current) {
        const bool deleting_last = index == nPages() - 1;
        const int go_to_index = deleting_last ? index - 1 : index + 1;
        deleting_current_and_moving_forward = !deleting_last;
        goToPage(go_to_index);  // alters m_current_pagenum_zero_based
    }

    m_pages.remove(index);

    if (deleting_earlier || deleting_current_and_moving_forward) {
        // We're not changing page, but the page number we're on is changing!
        m_current_page_index -= 1;
    }
}


void Questionnaire::movePageBackwards(const int index)
{
    if (index < 1 || index >= m_pages.size()) {
        return;
    }
    std::swap(m_pages[index - 1], m_pages[index]);
    goToPage(m_current_page_index);
}


void Questionnaire::movePageForwards(const int index)
{
    if (index < 0 || index >= m_pages.size() - 1) {
        return;
    }
    std::swap(m_pages[index], m_pages[index + 1]);
    goToPage(m_current_page_index);
}


void Questionnaire::goToPage(const int index, const bool allow_refresh)
{
    if (index < 0 || index >= nPages()) {
        qWarning() << Q_FUNC_INFO << "Invalid index:" << index;
        return;
    }
    if (index == m_current_page_index && !allow_refresh) {
        qDebug() << "Page" << index
                 << "(zero-based index) already selected";
        return;
    }
    pageClosing();
    m_current_page_index = index;
    build();
}


void Questionnaire::finishClicked()
{
    if (morePagesToGo()) {
        // Not on the last page; can't finish here
        return;
    }
    QuPage* page = currentPagePtr();
    if (!page) {
        return;
    }
    if (!readOnly()) {
        if (!page->mayProgressIgnoringValidators() || !page->validate()) {
            // Can't progress
            return;
        }
    }
    doFinish();
}


void Questionnaire::pageClosing()
{
    QuPage* page = currentPagePtr();
    if (!page) {
        return;
    }
    page->closing();
}


void Questionnaire::doCancel()
{
    if (!readOnly()) {
        // tell task about finish-with-abort
        emit editFinished(true);
    }
    emit cancelled();
    emit finished();
}


void Questionnaire::doFinish()
{
    if (!readOnly()) {
        // tell task about finish-without-abort
        emit editFinished(false);
    }
    emit completed();
    emit finished();
}


void Questionnaire::openSubWidget(OpenableWidget* widget)
{
#ifdef DEBUG_REPORT_OPEN_SUBWIDGET
    qDebug() << Q_FUNC_INFO;
#endif

    m_app.openSubWindow(widget);
}


CamcopsApp& Questionnaire::app() const
{
    return m_app;
}


QString Questionnaire::getSubstitutedCss(const QString& filename) const
{
    return m_app.getSubstitutedCss(filename);
}


void Questionnaire::debugLayout()
{
    layoutdumper::dumpWidgetHierarchy(this);
}


void Questionnaire::setVisibleByTag(const QString& tag, const bool visible,
                                    const bool current_page_only,
                                    const QString& page_tag)
{
    QVector<QuElement*> elements = getElementsByTag(tag, current_page_only,
                                                    page_tag);
    for (auto element : elements) {
        element->setVisible(visible);
    }
}


QVector<QuPage*> Questionnaire::getPages(const bool current_page_only,
                                         const QString& page_tag)
{
    QVector<QuPage*> pages;
    if (current_page_only) {
        QuPage* page = currentPagePtr();
        if (page && (page_tag.isEmpty() || page->hasTag(page_tag))) {
            pages.append(page);
        }
    } else {
        for (const QuPagePtr& p : m_pages) {
            if (page_tag.isEmpty() || p->hasTag(page_tag)) {
                pages.append(p.data());
            }
        }
    }
    return pages;
}


void Questionnaire::setPageSkip(const int page, const bool skip,
                                const bool reset_buttons)
{
    if (page < 0 || page >= m_pages.size()) {
        return;
    }
    m_pages[page]->setSkip(skip);
    if (reset_buttons) {
        resetButtons();
    }
}


void Questionnaire::setPageSkip(const QString& page_tag, const bool skip,
                                const bool reset_buttons)
{
    QVector<QuPage*> pages = getPages(false, page_tag);
    for (auto page : pages) {
        page->setSkip(skip);
    }
    if (reset_buttons) {
        resetButtons();
    }
}


QVector<QuElement*> Questionnaire::getElementsByTag(
        const QString& tag, const bool current_page_only,
        const QString& page_tag)
{
    const QVector<QuPage*> pages = getPages(current_page_only, page_tag);
    QVector<QuElement*> elements;
    for (auto page : pages) {
        elements += page->elementsWithTag(tag);
    }
    return elements;
}


QuElement* Questionnaire::getFirstElementByTag(
        const QString& tag, const bool current_page_only,
        const QString& page_tag)
{
    const QVector<QuElement*> elements = getElementsByTag(
                tag, current_page_only, page_tag);
    if (elements.isEmpty()) {
        return nullptr;
    }
    return elements.at(0);
}


void Questionnaire::setFinishButtonIcon(const QString& base_filename)
{
    m_finish_button_icon_base_filename = base_filename;
    if (m_p_header) {
        m_p_header->setFinishButtonIcon(base_filename);
    }
}


void Questionnaire::setFinishButtonIconToTick()
{
    setFinishButtonIcon(uiconst::CBS_OK);
}


QString Questionnaire::questionnaireStylesheet() const
{
    return m_app.getSubstitutedCss(uiconst::CSS_CAMCOPS_QUESTIONNAIRE);
}
