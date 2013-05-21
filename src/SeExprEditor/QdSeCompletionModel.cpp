/*
* (c) Disney Enterprises, Inc.  All rights reserved.
*
* This file is licensed under the terms of the Microsoft Public License (MS-PL)
* as defined at: http://opensource.org/licenses/MS-PL.
*
* A complete copy of this license is included in this distribution as the file
* LICENSE.
*
* @file QdSeCompletionModel.h
* @brief This provides an exression editor for SeExpr syntax with auto ui features
* @author  aselle
*/
#include <QtGui/QLineEdit>
#include <SeExpression.h>
#include <SeExprFunc.h>
#include "QdSeCompletionModel.h"

std::vector<QString> QdSeCompletionModel::builtins;

QdSeCompletionModel::QdSeCompletionModel(QObject* parent)
    :QAbstractItemModel(parent)
{
    if(builtins.size()==0){
        std::vector<std::string> builtins_std;
        SeExprFunc::getFunctionNames(builtins_std);
        for(unsigned int i=0;i<builtins_std.size();i++) builtins.push_back(QString(builtins_std[i].c_str()));
    }
}

void QdSeCompletionModel::clearVariables()
{
    variables.clear();variables_comment.clear();
}

void QdSeCompletionModel::addVariable(const QString& str,const QString& comment)
{
    variables.push_back(str);variables_comment.push_back(comment);
}

void QdSeCompletionModel::clearFunctions()
{
    functions.clear();functions_comment.clear();functionNameToFunction.clear();
}

void QdSeCompletionModel::addFunction(const QString& str,const QString& comment)
{
    functionNameToFunction[str]=functions_comment.size();
    functions.push_back(str);
    functions_comment.push_back(comment);
}

void QdSeCompletionModel::syncExtras(const QdSeCompletionModel& otherModel)
{
    functionNameToFunction=otherModel.functionNameToFunction;
    functions=otherModel.functions;
    functions_comment=otherModel.functions_comment;
    variables=otherModel.variables;
    variables_comment=otherModel.variables_comment;
}

QVariant QdSeCompletionModel::data(const QModelIndex& index,int role) const
{
    static QColor variableColor=QColor(100,200,250),functionColor=QColor(100,250,200),
        backgroundColor(50,50,50);

  if(!index.isValid()) return QVariant();
     int row=index.row(),column=index.column();

     int functions_offset=builtins.size();
     int variables_offset=functions_offset+functions.size();
     int local_variables_offset=variables_offset+variables.size();

     if(role==Qt::BackgroundRole) return backgroundColor;

     if(role==Qt::FontRole && column==0){
         QFont font;
         font.setBold(true);
         return font;
     }

     if(row<functions_offset){
         int index=row;
         if(role==Qt::DisplayRole || role==Qt::EditRole){
             if(column==0) return QVariant(builtins[index]);
             else if(column==1) return QVariant(getFirstLine(SeExprFunc::getDocString(builtins[index].toStdString().c_str())));
         }else if(role==Qt::ForegroundRole)
             return functionColor; // darkGreen;
     }else if(row<variables_offset){
         int index=row-functions_offset;
         if(role==Qt::DisplayRole || role==Qt::EditRole){
             if(column==0) return QVariant(functions[index]);
             else if(column==1) return QVariant(getFirstLine(functions_comment[index].toStdString()));
         }else if(role==Qt::ForegroundRole)
             return functionColor; //darkGreen;
     }else if(row<local_variables_offset){
         int index=row-variables_offset;
         if(role==Qt::DisplayRole || role==Qt::EditRole){
             if(column==0) return QVariant(variables[index]);
             else if(column==1) return QVariant(variables_comment[index]);
         }else if(role==Qt::ForegroundRole)
             return variableColor;
     }else if(row<local_variables_offset+(int)local_variables.size()){
         int index=row-local_variables_offset;
         if(role==Qt::DisplayRole || role==Qt::EditRole){
             if(column==0) return QVariant(local_variables[index]);
             else if(column==1) return QVariant("Local");
         }else if(role==Qt::ForegroundRole)
             return variableColor;
     }
     return QVariant();
}

QString QdSeCompletionModel::getDocString(const QString& s)
{
    std::map<QString,int>::iterator i=functionNameToFunction.find(s);
    if(i!=functionNameToFunction.end()){
        return functions_comment[i->second];
    }else 
        return SeExprFunc::getDocString(s.toStdString().c_str()).c_str();
}
