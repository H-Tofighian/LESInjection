/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "PatchInjection.H"
#include "TimeDataEntry.H"
#include "distributionModel.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class CloudType>
Foam::PatchInjection<CloudType>::PatchInjection
(
    const dictionary& dict,
    CloudType& owner,
    const word& modelName
)
:
    InjectionModel<CloudType>(dict, owner, modelName, typeName),
    patchInjectionBase(owner.mesh(), this->coeffDict().lookup("patchName")),
    ranGen_(label(0)),
    duration_(readScalar(this->coeffDict().lookup("duration"))),
    parcelsPerSecond_
    (
        readScalar(this->coeffDict().lookup("parcelsPerSecond"))
    ),
    U0_(this->coeffDict().lookup("U0")),
    ksgs_(readScalar(this->coeffDict().lookup("ksgs"))),
    si_(readScalar(this->coeffDict().lookup("si"))),
    flowRateProfile_
    (
        TimeDataEntry<scalar>
        (
            owner.db().time(),
            "flowRateProfile",
            this->coeffDict()
        )
    ),
    sizeDistribution_
    (
        distributionModels::distributionModel::New
        (
            this->coeffDict().subDict("sizeDistribution"),
            owner.rndGen()
        )
    ),
    UInterp_
    (
        interpolation<vector>::New
        (
            owner.solution().interpolationSchemes(),
            owner.U()
        )
    )
{
    duration_ = owner.db().time().userTimeToTime(duration_);

    patchInjectionBase::updateMesh(owner.mesh());

    // Set total volume/mass to inject
    this->volumeTotal_ = fraction_*flowRateProfile_.integrate(0.0, duration_);
    this->massTotal_ *= fraction_;
}


template<class CloudType>
Foam::PatchInjection<CloudType>::PatchInjection
(
    const PatchInjection<CloudType>& im
)
:
    InjectionModel<CloudType>(im),
    patchInjectionBase(im),
    ranGen_(im.ranGen_),
    duration_(im.duration_),
    parcelsPerSecond_(im.parcelsPerSecond_),
    U0_(im.U0_),
    ksgs_(im.ksgs_),
    si_(im.si_),
    flowRateProfile_(im.flowRateProfile_),
    sizeDistribution_(im.sizeDistribution_().clone().ptr()),
    UInterp_(im.UInterp_)
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

template<class CloudType>
Foam::PatchInjection<CloudType>::~PatchInjection()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class CloudType>
void Foam::PatchInjection<CloudType>::updateMesh()
{
    patchInjectionBase::updateMesh(this->owner().mesh());
}


template<class CloudType>
Foam::scalar Foam::PatchInjection<CloudType>::timeEnd() const
{
    return this->SOI_ + duration_;
}


template<class CloudType>
Foam::label Foam::PatchInjection<CloudType>::parcelsToInject
(
    const scalar time0,
    const scalar time1
)
{
    if ((time0 >= 0.0) && (time0 < duration_))
    {
        scalar nParcels = this->fraction_*(time1 - time0)*parcelsPerSecond_;

        cachedRandom& rnd = this->owner().rndGen();

        label nParcelsToInject = floor(nParcels);

        // Inject an additional parcel with a probability based on the
        // remainder after the floor function
        if
        (
            nParcelsToInject > 0
         && (
               nParcels - scalar(nParcelsToInject)
             > rnd.position(scalar(0), scalar(1))
            )
        )
        {
            ++nParcelsToInject;
        }

        return nParcelsToInject;
    }
    else
    {
        return 0;
    }
}


template<class CloudType>
Foam::scalar Foam::PatchInjection<CloudType>::volumeToInject
(
    const scalar time0,
    const scalar time1
)
{
    if ((time0 >= 0.0) && (time0 < duration_))
    {
        return this->fraction_*flowRateProfile_.integrate(time0, time1);
    }
    else
    {
        return 0.0;
    }
}


template<class CloudType>
void Foam::PatchInjection<CloudType>::setPositionAndCell
(
    const label,
    const label,
    const scalar,
    vector& position,
    label& cellOwner,
    label& tetFaceI,
    label& tetPtI
)
{
    patchInjectionBase::setPositionAndCell
    (
        this->owner().mesh(),
        this->owner().rndGen(),
        position,
        cellOwner,
        tetFaceI,
        tetPtI
    );
}


template<class CloudType>
void Foam::PatchInjection<CloudType>::setProperties
(
    const label,
    const label,
    const scalar,
    typename CloudType::parcelType& parcel
)
{

    // set particle diameter
    parcel.d() = sizeDistribution_->sample();
	//calculate particle velocity
    parcel.U() = UInterp_->interpolate(parcel.position(), parcel.currentTetIndices())*si_;
    if (si_ == 0.0)
    {
		parcel.U() = U0_;
	}
	// calculate fluid phase flactuating velocity (isotropic) for dispertion models
	//~ vector randv;
	//~ randv.x() = ranGen_.GaussNormal();
	//~ randv.y() = ranGen_.GaussNormal();
	//~ randv.z() = ranGen_.GaussNormal();
	//~ vector uf = sqrt(2*ksgs_/3.0) * randv;
	// set fluid phase flactuating velocity
    //~ parcel.UTurb() = uf;
}

template<class CloudType>
bool Foam::PatchInjection<CloudType>::fullyDescribed() const
{
    return false;
}


template<class CloudType>
bool Foam::PatchInjection<CloudType>::validInjection(const label)
{
    return true;
}


// ************************************************************************* //
