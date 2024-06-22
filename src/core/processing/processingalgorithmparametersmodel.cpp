/***************************************************************************
  processingalgorithmparametersmodel.h - ProcessingAlgorithmParametersModel

 ---------------------
 begin                : 21.06.2024
 copyright            : (C) 2024 by Mathieu Pellerin
 email                : mathieu at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "processingalgorithmparametersmodel.h"

#include <qgsapplication.h>
#include <qgsprocessingalgorithm.h>
#include <qgsprocessingparameters.h>
#include <qgsprocessingregistry.h>


ProcessingAlgorithmParametersModel::ProcessingAlgorithmParametersModel( QObject *parent )
  : QSortFilterProxyModel( parent )
  , mModel( new ProcessingAlgorithmParametersModelBase( parent ) )
{
  setSourceModel( mModel );
  connect( mModel, &ProcessingAlgorithmParametersModelBase::algorithmIdChanged, this, &ProcessingAlgorithmParametersModel::algorithmIdChanged );
}

void ProcessingAlgorithmParametersModel::setFilters( ProcessingAlgorithmParametersModel::Filters filters )
{
  if ( mFilters == filters )
  {
    return;
  }

  mFilters = filters;
  emit filtersChanged();

  invalidateFilter();
}

QString ProcessingAlgorithmParametersModel::algorithmId() const
{
  return mModel->algorithmId();
}

void ProcessingAlgorithmParametersModel::setAlgorithmId( const QString &id )
{
  mModel->setAlgorithmId( id );
}

bool ProcessingAlgorithmParametersModel::isValid() const
{
  return mModel->isValid();
}

bool ProcessingAlgorithmParametersModel::hasAdvancedParameters() const
{
  return mModel->hasAdvancedParameters();
}

QString ProcessingAlgorithmParametersModel::algorithmDisplayName() const
{
  return mModel->algorithmDisplayName();
}

QString ProcessingAlgorithmParametersModel::algorithmShortHelp() const
{
  return mModel->algorithmShortHelp();
}

bool ProcessingAlgorithmParametersModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
{
  QModelIndex sourceIndex = mModel->index( sourceRow, 0, sourceParent );
  if ( ( mFilters & Filter::GeneralParameterFilter ) && ( mFilters & Filter::AdvancedParameterFilter ) )
  {
    return true;
  }
  else if ( mFilters & Filter::GeneralParameterFilter )
  {
    const bool advancedParameter = mModel->data( sourceIndex, ProcessingAlgorithmParametersModelBase::ParameterFlagsRole ).toInt() & static_cast<int>( Qgis::ProcessingParameterFlag::Advanced );
    if ( advancedParameter )
      return false;
  }
  else if ( mFilters & Filter::AdvancedParameterFilter )
  {
    const bool advancedParameter = mModel->data( sourceIndex, ProcessingAlgorithmParametersModelBase::ParameterFlagsRole ).toInt() & static_cast<int>( Qgis::ProcessingParameterFlag::Advanced );
    if ( !advancedParameter )
      return false;
  }

  return true;
}


ProcessingAlgorithmParametersModelBase::ProcessingAlgorithmParametersModelBase( QObject *parent )
  : QAbstractListModel( parent )
{
}

void ProcessingAlgorithmParametersModelBase::rebuild()
{
  beginResetModel();
  mHasAdvancedParameters = false;
  mParameters.clear();
  mValues.clear();

  if ( mAlgorithm )
  {
    const static QStringList sSupportedParameters = { QStringLiteral( "number" ) };
    const QgsProcessingAlgorithm *algorithm = QgsApplication::instance()->processingRegistry()->algorithmById( mAlgorithmId );
    for ( const QgsProcessingParameterDefinition *definition : algorithm->parameterDefinitions() )
    {
      if ( sSupportedParameters.contains( definition->type() ) )
      {
        if ( definition->flags() & Qgis::ProcessingParameterFlag::Advanced )
        {
          mHasAdvancedParameters = true;
        }

        mParameters << definition;
        mValues << definition->defaultValue();
      }
    }
  }

  endResetModel();
}

void ProcessingAlgorithmParametersModelBase::setAlgorithmId( const QString &id )
{
  if ( mAlgorithmId == id )
  {
    return;
  }

  mAlgorithmId = id;
  mAlgorithm = !mAlgorithmId.isEmpty() ? QgsApplication::instance()->processingRegistry()->algorithmById( mAlgorithmId ) : nullptr;

  rebuild();

  emit algorithmIdChanged();
}

QString ProcessingAlgorithmParametersModelBase::algorithmDisplayName() const
{
  return mAlgorithm ? mAlgorithm->displayName() : QString();
}

QString ProcessingAlgorithmParametersModelBase::algorithmShortHelp() const
{
  return mAlgorithm ? mAlgorithm->shortHelpString() : QString();
}

QHash<int, QByteArray> ProcessingAlgorithmParametersModelBase::roleNames() const
{
  QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
  roles[ParameterTypeRole] = "ParameterType";
  roles[ParameterDescriptionRole] = "ParameterDescription";
  roles[ParameterFlagsRole] = "ParameterFlags";
  roles[ParameterDefaultValueRole] = "ParameterDefaultValue";
  roles[ParameterValueRole] = "ParameterValue";

  return roles;
}

int ProcessingAlgorithmParametersModelBase::rowCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() )
    return mParameters.size();
  else
    return 0;
}

QVariant ProcessingAlgorithmParametersModelBase::data( const QModelIndex &index, int role ) const
{
  if ( index.row() >= mParameters.size() || index.row() < 0 || !mParameters.at( index.row() ) )
    return QVariant();

  switch ( role )
  {
    case ParameterTypeRole:
      return mParameters.at( index.row() )->type();
    case ParameterDescriptionRole:
      return mParameters.at( index.row() )->description();
    case ParameterFlagsRole:
      return static_cast<int>( mParameters.at( index.row() )->flags() );
    case ParameterDefaultValueRole:
      return mParameters.at( index.row() )->defaultValue();
    case ParameterValueRole:
      return mValues.at( index.row() );
  }

  return QVariant();
}

bool ProcessingAlgorithmParametersModelBase::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( index.row() >= mParameters.size() || index.row() < 0 || !mParameters.at( index.row() ) )
    return false;

  switch ( role )
  {
    case ParameterValueRole:
      mValues[index.row()] = value;
      return true;
  }

  return false;
}
