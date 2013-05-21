/*
* (c) Disney Enterprises, Inc.  All rights reserved.
*
* This file is licensed under the terms of the Microsoft Public License (MS-PL)
* as defined at: http://opensource.org/licenses/MS-PL.
*
* A complete copy of this license is included in this distribution as the file
* LICENSE.
*
* @file QdSeShortEdit.cpp
* @brief This provides an exression editor for SeExpr syntax with auto ui features
* @author  aselle
*/
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QIcon>
#include <QtGui/QCompleter>
#include <QtGui/QTreeView>
#include <QtGui/QScrollBar>
#include <QtGui/QToolTip>
#include <QtGui/QLabel>


#include "QdSeShortEdit.h"
#include "QdSeDialog.h"
#include "QdSeBrowser.h"
#include "QdSeHighlighter.h"
#include "QdSeCompletionModel.h"
#include "QdSeControlCollection.h"
#include "QdSePopupDocumentation.h"
#include "QdSeExpr.h"


/* XPM */
static const char *sum_xpm[]={
"16 16 6 1",
"# c None",
". c None",
"b c #808080",
"d c #010000",
"c c #aaaaaa",
"a c #303030",
"................",
".#aaaaaaaaaa##..",
".abbbbbbcbbba...",
".#abbaaaaabba...",
"..#aabba..aba...",
"....#abba..a#...",
".....#abba......",
".....#abba......",
"...##abba...#...",
"...#abba...aa...",
"..#abba...aca...",
".#abbaaaaabba...",
".abbbbbbbbbba...",
".aaaaaaaaaaa#...",
"................",
"................"};

/* XPM */
static const char *stop_xpm[] = {
"16 16 4 1",
"       c None",
".      c #FF0000",
"+      c #FF8080",
"@      c #FFFFFF",
"                ",
"                ",
"     ......     ",
"    ...+++..    ",
"   ....@@@...   ",
"  .....@@@....  ",
"  .....@@@....  ",
"  .....@@@....  ",
"  .....@@@....  ",
"  ............  ",
"   ....@@@....  ",
"    ...@@@...   ",
"     .......    ",
"      .....     ",
"                ",
"                "};



QdSeShortEdit::QdSeShortEdit(QWidget* parent, bool expanded)
    :QWidget(parent), _context(""), _searchPath("")
{
    controlRebuildTimer=new QTimer();

    vboxlayout=new QVBoxLayout();
    vboxlayout->setMargin(0);
    hboxlayout=new QHBoxLayout();
    hboxlayout->setMargin(0);
    //edit=new QdSeShortLineEdit();
    //edit=new QLineEdit();
    edit=new QdSeShortTextEdit(parent);

    error=new QLabel();
    error->setPixmap(QPixmap(stop_xpm));
    error->setHidden(true);
    
    expandButton=new QToolButton;
    expandButton->setMinimumSize(20,20);
    expandButton->setMaximumSize(20,20);
    expandButton->setFocusPolicy(Qt::NoFocus);
    if (expanded) expandButton->setArrowType(Qt::DownArrow);
    else expandButton->setArrowType(Qt::RightArrow);
    connect(expandButton,SIGNAL(clicked()),SLOT(expandPressed()));

    QToolButton* button=new QToolButton;
    editDetail=button;
    button->setIcon(QIcon(QPixmap(sum_xpm)));
    hboxlayout->addWidget(expandButton);
    hboxlayout->addWidget(edit);
    hboxlayout->addWidget(error);
    hboxlayout->addWidget(editDetail);

    editDetail->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    editDetail->setMinimumSize(20,20);
    editDetail->setMaximumSize(20,20);
    connect(editDetail,SIGNAL(clicked()),SLOT(detailPressed()));
    //connect(edit,SIGNAL(textChanged()),SLOT(handleTextEdited()));
    connect(edit,SIGNAL(editingFinished()),SLOT(textFinished()));

    vboxlayout->addLayout(hboxlayout);

    controls=0;
    if (expanded) expandPressed();

    setLayout(vboxlayout);
    connect(controlRebuildTimer, SIGNAL(timeout()), SLOT(rebuildControls()));
}

QdSeShortEdit::~QdSeShortEdit()
{
    delete controlRebuildTimer;
}

void QdSeShortEdit::setSearchPath(const QString& context, const QString& path)
{
    _context = context.toStdString();
    _searchPath = path.toStdString();
}

void QdSeShortEdit::detailPressed()
{
    showDetails(-1);
}

int QdSeShortEdit::showDetails(int idx)
{
    QdSeDialog dialog(0);
    dialog.editor->replaceExtras(*edit->completionModel);

    dialog.browser->setSearchPath(_context.c_str(), _searchPath.c_str());
    dialog.browser->expandAll();
    dialog.setExpressionString(getExpressionString());
    if (idx >= 0) {
        dialog.showEditor(idx);
    }

    int result = dialog.exec();

    if (QDialog::Accepted == result) {
        setExpressionString(dialog.getExpressionString());
    }

    return result;
}

void QdSeShortEdit::rebuildControls()
{
    if(controls){
	bool wasShown=!edit->completer->popup()->isHidden();
        //std::vector<QString> variables;
        bool newVariables=controls->rebuildControls(getExpression(),edit->completionModel->local_variables);
        if (controls->numControls()==0)
	{
            controls->deleteLater();
            controls=0;
            expandButton->setArrowType(Qt::RightArrow);
	}
	else vboxlayout->addWidget(controls);
        if(newVariables) edit->completer->setModel(edit->completionModel);
        if(wasShown) edit->completer->popup()->show();
    }
}

void QdSeShortEdit::expandPressed()
{
    if(controls){
        //vboxlayout->removeWidget(controls);
        controls->deleteLater();
        controls=0;
        expandButton->setArrowType(Qt::RightArrow);
    }else{
        controls=new QdSeControlCollection(0,false);
        //vboxlayout->addWidget(controls);
        connect(controls,SIGNAL(controlChanged(int)),SLOT(controlChanged(int)));
        controlRebuildTimer->setSingleShot(true);
        controlRebuildTimer->start(0);
        expandButton->setArrowType(Qt::DownArrow);
    }
}

void QdSeShortEdit::handleTextEdited()
{}

void QdSeShortEdit::textFinished()
{
    controlRebuildTimer->setSingleShot(true);
    controlRebuildTimer->start(0);
    checkErrors();
    emit exprChanged();
}

void QdSeShortEdit::setExpressionString(const std::string& expression)
{
    edit->setText(QString(expression.c_str()).replace("\n","\\n"));
    //rebuildControls();
    controlRebuildTimer->setSingleShot(true);
    controlRebuildTimer->start(0);
    checkErrors();
    emit exprChanged();
}

QString QdSeShortEdit::getExpression() const
{
    return edit->toPlainText().replace("\\n","\n");
}

std::string QdSeShortEdit::getExpressionString()const
{ 
    return getExpression().toStdString();
}

void QdSeShortEdit::controlChanged(int id)
{
    if(controls){
        QString newText=getExpression();
        controls->updateText(id,newText);
        edit->setText(newText.replace("\n","\\n"));
        checkErrors();
        emit exprChanged();
    }
}

void QdSeShortEdit::
clearExtraCompleters()
{
    edit->completionModel->clearFunctions();
    edit->completionModel->clearVariables();
}

void QdSeShortEdit::
registerExtraFunction(const std::string& name,const std::string& docString)
{
    edit->completionModel->addFunction(name.c_str(),docString.c_str());
}

void QdSeShortEdit::
registerExtraVariable(const std::string& name,const std::string& docString)
{
    edit->completionModel->addVariable(name.c_str(),docString.c_str());
}

void QdSeShortEdit::
updateCompleter()
{
    edit->completer->setModel(edit->completionModel);
}

void QdSeShortEdit::
checkErrors()
{
    QdSeExpr expr(getExpressionString(),true);
    bool valid=expr.isValid();
    std::string err;
    if (!valid)
        err = expr.parseError();

    hideErrors(valid, err);
}

void QdSeShortEdit::
hideErrors(bool hidden, const std::string &err)
{
    error->setHidden(hidden);
    if (!hidden) {
        error->setToolTip(QString::fromStdString(err));
    }
}

void QdSeShortEdit::
setSimple(bool enabled)
{
    edit->setHidden(enabled);
    editDetail->setHidden(enabled);
    expandButton->setHidden(enabled);
}

void QdSeShortEdit::setDetailsMenu(QMenu *menu)
{
    editDetail->setMenu(menu);
}

void QdSeShortEdit::setVerticalScrollBarPolicy(Qt::ScrollBarPolicy policy)
{
    edit->setVerticalScrollBarPolicy(policy);
}


QdSeShortTextEdit::QdSeShortTextEdit(QWidget* parent)
:QTextEdit(parent),editing(false),_tip(0)
{
    lastStyleForHighlighter=0;
    setMaximumHeight(25);
    highlighter=new QdSeHighlighter(document());
    highlighter->fixStyle(palette());
    highlighter->rehighlight();
    repaint();

    // setup auto completion
    completer=new QCompleter();
    completionModel=new QdSeCompletionModel(this);
    completer->setModel(completionModel);
    QTreeView* treePopup=new QTreeView;
    completer->setPopup(treePopup);
    treePopup->setRootIsDecorated(false);
    treePopup->setMinimumWidth(300);
    treePopup->setMinimumHeight(50);
    treePopup->setItemsExpandable(true);

    completer->setWidget(this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(completer,SIGNAL(activated(const QString&)),this,
        SLOT(insertCompletion(const QString&)));

}

void QdSeShortTextEdit::focusInEvent(QFocusEvent* e)
{
    //setTextCursor(QTextCursor(document()));
    if(completer) completer->setWidget(this);
    QTextEdit::focusInEvent(e);
}

void QdSeShortTextEdit::focusOutEvent(QFocusEvent* e)
{
    //setTextCursor(QTextCursor());
    finishEdit();
    QTextCursor newCursor=textCursor();
    newCursor.clearSelection();
    setTextCursor(newCursor);
    setColor(false);
    hideTip();
    QTextEdit::focusOutEvent(e);
}

void QdSeShortTextEdit::mousePressEvent(QMouseEvent* event)
{
    hideTip();
    QTextEdit::mousePressEvent(event);
}

void QdSeShortTextEdit::mouseDoubleClickEvent(QMouseEvent* event)
{
    hideTip();
    QTextEdit::mouseDoubleClickEvent(event);
}

void QdSeShortTextEdit::paintEvent(QPaintEvent* e){
    if(lastStyleForHighlighter!=style()){
        lastStyleForHighlighter=style();
        highlighter->fixStyle(palette());
        highlighter->rehighlight();
    }
    QTextEdit::paintEvent(e);
}


void QdSeShortTextEdit::keyPressEvent( QKeyEvent* e )
{


    // If the completer is active pass keys it needs down
    if(completer && completer->popup()->isVisible()){
        switch(e->key()){
            case Qt::Key_Enter:case Qt::Key_Return:case Qt::Key_Escape:
            case Qt::Key_Tab:case Qt::Key_Backtab:
                e->ignore();return;
            default:
                break;
        }
    }

    // Accept expression
    if (e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter){
        selectAll();
        finishEdit();
        return;
    }else if (e->key()==Qt::Key_Escape){
        setText(savedText);
        selectAll();
        finishEdit();
        return;
    }else if(e->key()==Qt::Key_Tab){
        QWidget::keyPressEvent(e);
        return;
    }else if(!editing){
        editing=true;
        setColor(true);
        savedText=toPlainText();
    }



    // use the values here as long as we are not using the shortcut to bring up the editor
    bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (!isShortcut) // dont process the shortcut when we have a completer
      QTextEdit::keyPressEvent(e);
    

    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (!completer || (ctrlOrShift && e->text().isEmpty()))
        return;

    bool hasModifier=(e->modifiers()!=Qt::NoModifier) && !ctrlOrShift;

    // grab the line we're on 
    QTextCursor tc=textCursor();
    tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    QString line=tc.selectedText();

    // matches the last prefix of a completable variable or function and extract as completionPrefix
    static QRegExp completion("^(?:.*[^A-Za-z0-9_$])?((?:\\$[A-Za-z0-9_]*)|[A-Za-z]+[A-Za-z0-9_]*)$");
    int index=completion.indexIn(line);
    QString completionPrefix;
    if(index != -1 && !line.contains('#')){
        completionPrefix=completion.cap(1);
        //std::cout<<"we have completer prefix '"<<completionPrefix.toStdString()<<"'"<<std::endl;
    }

    // hide the completer if we have too few characters, we are at end of word
    if(!isShortcut && (hasModifier || e->text().isEmpty() || completionPrefix.length()<1 || index==-1)){
        completer->popup()->hide();
    }else{
        
        // copy the completion prefix in if we don't already have it in the completer
        if(completionPrefix!=completer->completionPrefix()){
            completer->setCompletionPrefix(completionPrefix);
            completer->popup()->setCurrentIndex(completer->completionModel()->index(0,0));
        }
        
        // display the completer
       QRect cr=cursorRect();
       cr.setWidth(2*(completer->popup()->sizeHintForColumn(0)+completer->popup()->sizeHintForColumn(1)
                + completer->popup()->verticalScrollBar()->sizeHint().width()));
        completer->complete(cr);
        hideTip();
        return;
    }

    // documentation completion
    static QRegExp inFunction("^(?:.*[^A-Za-z0-9_$])?([A-Za-z0-9_]+)\\([^()]*$");
    int index2=inFunction.indexIn(line);
    if(index2!=-1){
        QString functionName=inFunction.cap(1);
        QStringList tips=completionModel->getDocString(functionName).split("\n");
        QString tip="<b>"+tips[0]+"</b>";
        for(int i=1;i<tips.size();i++){
            tip+="<br>"+tips[i];
        }
        showTip(tip);
    }else{
        hideTip();
    }

}

void QdSeShortTextEdit::showTip(const QString& string)
{
    // skip empty strings
    if(string=="") return;
    // skip already shwon stuff
    //if(_tip && !_tip->isHidden() && _tip->label->text() == string) return;

    QRect cr=cursorRect();
    cr.setX(0);
    cr.setWidth(cr.width()*3);
    if(_tip){delete _tip;_tip=0;}
    _tip=new QdSePopupDocumentation(this,mapToGlobal(cr.bottomLeft()),string);
}

void QdSeShortTextEdit::hideTip()
{
    if(_tip) _tip->hide();
}

void QdSeShortTextEdit::insertCompletion(const QString &completion)
{
    if (completer->widget() != this) return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    if(completion[0]!='$') tc.insertText("(");
    setTextCursor(tc);

}

void QdSeShortTextEdit::finishEdit()
{
    editing=false;
    setColor(false);
    emit editingFinished();
}



void QdSeShortTextEdit::
setColor(bool editing)
{
    // todo: decorate when editing
}
