#include "vtagpanel.h"

#include <QtWidgets>

#include "vbuttonwithwidget.h"
#include "utils/viconutils.h"
#include "vnotefile.h"
#include "valltagspanel.h"
#include "vtaglabel.h"
#include "vlineedit.h"
#include "vmainwindow.h"

extern VPalette *g_palette;

extern VMainWindow *g_mainWin;

#define MAX_DISPLAY_LABEL 3

VTagPanel::VTagPanel(QWidget *parent)
    : QWidget(parent),
      m_file(NULL),
      m_notebookOfCompleter(NULL)
{
    setupUI();
}

void VTagPanel::setupUI()
{
    for (int i = 0; i < MAX_DISPLAY_LABEL; ++i) {
        VTagLabel *label = new VTagLabel(this);
        connect(label, &VTagLabel::removalRequested,
                this, [this](const QString &p_text) {
                    removeTag(p_text);
                    updateTags();
                });

        m_labels.append(label);
    }

    m_tagsPanel = new VAllTagsPanel(this);
    connect(m_tagsPanel, &VAllTagsPanel::tagRemoved,
            this, [this](const QString &p_text) {
                removeTag(p_text);

                if (m_file->getTags().size() <= MAX_DISPLAY_LABEL) {
                    // Hide the more panel.
                    m_btn->hidePopupWidget();
                    m_btn->hide();
                }
            });

    m_btn = new VButtonWithWidget(VIconUtils::icon(":/resources/icons/tags.svg",
                                                   g_palette->color("tab_indicator_tag_label_bg")),
                                  "",
                                  m_tagsPanel,
                                  this);
    m_btn->setToolTip(tr("View and edit tags of current note"));
    m_btn->setProperty("StatusBtn", true);
    m_btn->setFocusPolicy(Qt::NoFocus);
    connect(m_btn, &VButtonWithWidget::popupWidgetAboutToShow,
            this, &VTagPanel::updateAllTagsPanel);

    m_tagEdit = new VLineEdit(this);
    m_tagEdit->setToolTip(tr("Press Enter to add a tag"));
    m_tagEdit->setPlaceholderText(tr("Add a tag"));
    m_tagEdit->setProperty("EmbeddedEdit", true);
    m_tagEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    connect(m_tagEdit, &QLineEdit::returnPressed,
            this, [this]() {
                Q_ASSERT(m_file);
                QString text = m_tagEdit->text();
                if (addTag(text)) {
                    if (m_file->getNotebook()->addTag(text)) {
                        updateCompleter();
                    }

                    updateTags();

                    g_mainWin->showStatusMessage(tr("Tag \"%1\" added").arg(text));
                }

                m_tagEdit->clear();
            });

    QValidator *validator = new QRegExpValidator(QRegExp("[^,]+"), m_tagEdit);
    m_tagEdit->setValidator(validator);

    // Completer.
    m_tagsModel = new QStringListModel(this);
    QCompleter *completer = new QCompleter(m_tagsModel, this);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->popup()->setItemDelegate(new QStyledItemDelegate(this));
    m_tagEdit->setCompleter(completer);
    m_tagEdit->installEventFilter(this);

    QHBoxLayout *mainLayout = new QHBoxLayout();
    for (auto label : m_labels) {
        mainLayout->addWidget(label);
        label->hide();
    }

    mainLayout->addWidget(m_btn);
    mainLayout->addWidget(m_tagEdit);

    mainLayout->setContentsMargins(0, 0, 0, 0);

    setLayout(mainLayout);
}

void VTagPanel::clearLabels()
{
    for (auto label : m_labels) {
        label->clear();
        label->hide();
    }

    m_tagsPanel->clear();
}

void VTagPanel::updateTags(VNoteFile *p_file)
{
    if (m_file == p_file) {
        return;
    }

    m_file = p_file;

    updateTags();
}

void VTagPanel::updateTags()
{
    clearLabels();

    if (!m_file) {
        m_btn->setVisible(false);
        return;
    }

    const QStringList &tags = m_file->getTags();
    int idx = 0;
    for (; idx < tags.size() && idx < m_labels.size(); ++idx) {
        m_labels[idx]->setText(tags[idx]);
        m_labels[idx]->show();
    }

    m_btn->setVisible(idx < tags.size());
}

void VTagPanel::updateAllTagsPanel()
{
    Q_ASSERT(m_file);
    m_tagsPanel->clear();

    const QStringList &tags = m_file->getTags();
    for (int idx = MAX_DISPLAY_LABEL; idx < tags.size(); ++idx) {
        m_tagsPanel->addTag(tags[idx]);
    }
}

void VTagPanel::removeTag(const QString &p_text)
{
    if (!m_file) {
        return;
    }

    m_file->removeTag(p_text);
    g_mainWin->showStatusMessage(tr("Tag \"%1\" removed").arg(p_text));
}

bool VTagPanel::addTag(const QString &p_text)
{
    if (!m_file) {
        return false;
    }

    bool ret = m_file->addTag(p_text);
    return ret;
}

bool VTagPanel::eventFilter(QObject *p_obj, QEvent *p_event)
{
    Q_ASSERT(p_obj == m_tagEdit);

    if (p_event->type() == QEvent::FocusIn) {
        QFocusEvent *eve = static_cast<QFocusEvent *>(p_event);
        if (eve->gotFocus()) {
            // Just check completer.
            updateCompleter(m_file);
        }
    }

    return QWidget::eventFilter(p_obj, p_event);
}

void VTagPanel::updateCompleter(const VNoteFile *p_file)
{
    const VNotebook *nb = p_file ? p_file->getNotebook() : NULL;
    if (nb == m_notebookOfCompleter) {
        // No need to update.
        return;
    }

    m_notebookOfCompleter = nb;
    updateCompleter();
}

void VTagPanel::updateCompleter()
{
    m_tagsModel->setStringList(m_notebookOfCompleter ? m_notebookOfCompleter->getTags()
                                                     : QStringList());
}
