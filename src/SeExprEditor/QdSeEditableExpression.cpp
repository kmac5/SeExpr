/*
* (c) Disney Enterprises, Inc.  All rights reserved.
*
* This file is licensed under the terms of the Microsoft Public License (MS-PL)
* as defined at: http://opensource.org/licenses/MS-PL.
*
* A complete copy of this license is included in this distribution as the file
* LICENSE.
*/
#include "QdSeEditable.h"
#include "QdSeEditableExpression.h"
#include <sstream>

bool SeExprParse(std::vector<QdSeEditable*>& literals,std::vector<std::string>& variables,
    std::vector<std::pair<int,int> >& comments,const char* str);


QdSeEditableExpression::QdSeEditableExpression()
{}

QdSeEditableExpression::~QdSeEditableExpression()
{
    cleanup();
}

void QdSeEditableExpression::
setExpr(const std::string& expr)
{
    // get rid of old data
    cleanup();

    // run parser
    _expr=expr;
    std::vector<std::pair<int,int> > comments;
    SeExprParse(_editables,_variables,comments,_expr.c_str());

    for(Editables::iterator it=_editables.begin();it!=_editables.end();){
        QdSeEditable& literal=**it;
        int endPos=literal.endPos;
        std::string comment="";
        for(size_t ci=0;ci<comments.size();ci++){
            if(comments[ci].first>=endPos){
                // check to make sure there is no newlines between end of editable and comment
                size_t pos=_expr.find('\n',endPos);
                if(pos==std::string::npos || pos>=(size_t)comments[ci].second){
                    comment=_expr.substr(comments[ci].first,comments[ci].second-comments[ci].first);
                    break;
                }
            }
        }
        bool keepEditable=literal.parseComment(comment);
        if(!keepEditable){ // TODO: this is potentially quadratic if we remove a bunch
            delete &literal;
            it=_editables.erase(it);
        }else{
            ++it;
        }
    }
}

void QdSeEditableExpression::cleanup()
{
    for(size_t i=0;i<_editables.size();i++) delete _editables[i];
    _editables.clear();
    _variables.clear();

}

std::string QdSeEditableExpression::getEditedExpr() const
{
    int offset=0;
    std::stringstream stream;
    for(size_t i=0,sz=_editables.size();i<sz;i++){
        QdSeEditable& literal=*_editables[i];
        stream<<_expr.substr(offset,literal.startPos-offset);
        literal.appendString(stream);
        offset=literal.endPos;
    }
    stream<<_expr.substr(offset,_expr.size()-offset);
    return stream.str();
}

void QdSeEditableExpression::updateString(const QdSeEditableExpression& other)
{
    // TODO: move semantics?
    _variables=other._variables;
    _expr=other._expr;
    _variables=other._variables;
    for(size_t i=0,sz=_editables.size();i<sz;i++){
        QdSeEditable& literal=*_editables[i];
        QdSeEditable& otherLiteral=*other._editables[i];
        assert(literal.controlsMatch(otherLiteral));
        literal.updatePositions(otherLiteral);
    }
}

bool QdSeEditableExpression::controlsMatch(const QdSeEditableExpression& other) const
{
    if(_editables.size() != other._editables.size()) return false;

    for(size_t i=0,sz=_editables.size();i<sz;i++){
        const QdSeEditable& literal=*_editables[i];
        const QdSeEditable& otherLiteral=*other._editables[i];
        if(!literal.controlsMatch(otherLiteral)) return false;
    }
    return true;
}
