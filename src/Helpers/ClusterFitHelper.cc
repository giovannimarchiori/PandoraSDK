/**
 *  @file   PandoraSDK/src/Helpers/ClusterFitHelper.cc
 * 
 *  @brief  Implementation of the cluster fit helper class.
 * 
 *  $Log: $
 */

#include "Helpers/ClusterFitHelper.h"

#include "Objects/Cluster.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace pandora
{

StatusCode ClusterFitHelper::FitStart(const Cluster *const pCluster, const unsigned int maxOccupiedLayers, ClusterFitResult &clusterFitResult)
{
    if (maxOccupiedLayers < 2)
        return STATUS_CODE_INVALID_PARAMETER;

    const OrderedCaloHitList &orderedCaloHitList = pCluster->GetOrderedCaloHitList();
    const unsigned int listSize(orderedCaloHitList.size());

    if (0 == listSize)
        return STATUS_CODE_NOT_INITIALIZED;

    if (listSize < 2)
        return STATUS_CODE_OUT_OF_RANGE;

    unsigned int occupiedLayerCount(0);

    ClusterFitPointList clusterFitPointList;
    for (const OrderedCaloHitList::value_type &layerIter : orderedCaloHitList)
    {
        if (++occupiedLayerCount > maxOccupiedLayers)
            break;

        for (const CaloHit *const pCaloHit : *layerIter.second)
        {
            clusterFitPointList.push_back(ClusterFitPoint(pCaloHit));
        }
    }

    return FitPoints(clusterFitPointList, clusterFitResult);
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ClusterFitHelper::FitEnd(const Cluster *const pCluster, const unsigned int maxOccupiedLayers, ClusterFitResult &clusterFitResult)
{
    if (maxOccupiedLayers < 2)
        return STATUS_CODE_INVALID_PARAMETER;

    const OrderedCaloHitList &orderedCaloHitList = pCluster->GetOrderedCaloHitList();
    const unsigned int listSize(orderedCaloHitList.size());

    if (0 == listSize)
        return STATUS_CODE_NOT_INITIALIZED;

    if (listSize < 2)
        return STATUS_CODE_OUT_OF_RANGE;

    unsigned int occupiedLayerCount(0);

    ClusterFitPointList clusterFitPointList;
    for (OrderedCaloHitList::const_reverse_iterator iter = orderedCaloHitList.rbegin(), iterEnd = orderedCaloHitList.rend(); iter != iterEnd; ++iter)
    {
        if (++occupiedLayerCount > maxOccupiedLayers)
            break;

        for (const CaloHit *const pCaloHit : *iter->second)
        {
            clusterFitPointList.push_back(ClusterFitPoint(pCaloHit));
        }
    }

    return FitPoints(clusterFitPointList, clusterFitResult);
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ClusterFitHelper::FitFullCluster(const Cluster *const pCluster, ClusterFitResult &clusterFitResult)
{
    const OrderedCaloHitList &orderedCaloHitList = pCluster->GetOrderedCaloHitList();
    const unsigned int listSize(orderedCaloHitList.size());

    if (0 == listSize)
        return STATUS_CODE_NOT_INITIALIZED;

    if (listSize < 2)
        return STATUS_CODE_OUT_OF_RANGE;

    ClusterFitPointList clusterFitPointList;
    for (const OrderedCaloHitList::value_type &layerIter : orderedCaloHitList)
    {
        for (const CaloHit *const pCaloHit : *layerIter.second)
        {
            clusterFitPointList.push_back(ClusterFitPoint(pCaloHit));
        }
    }

    return FitPoints(clusterFitPointList, clusterFitResult);
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ClusterFitHelper::FitLayers(const Cluster *const pCluster, const unsigned int startLayer, const unsigned int endLayer,
    ClusterFitResult &clusterFitResult)
{
    if (startLayer >= endLayer)
        return STATUS_CODE_INVALID_PARAMETER;

    const OrderedCaloHitList &orderedCaloHitList = pCluster->GetOrderedCaloHitList();
    const unsigned int listSize(orderedCaloHitList.size());

    if (0 == listSize)
        return STATUS_CODE_NOT_INITIALIZED;

    if (listSize < 2)
        return STATUS_CODE_OUT_OF_RANGE;

    ClusterFitPointList clusterFitPointList;
    for (const OrderedCaloHitList::value_type &layerIter : orderedCaloHitList)
    {
        const unsigned int pseudoLayer(layerIter.first);

        if (startLayer > pseudoLayer)
            continue;

        if (endLayer < pseudoLayer)
            break;

        for (const CaloHit *const pCaloHit : *layerIter.second)
        {
            clusterFitPointList.push_back(ClusterFitPoint(pCaloHit));
        }
    }

    return FitPoints(clusterFitPointList, clusterFitResult);
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ClusterFitHelper::FitLayerCentroids(const Cluster *const pCluster, const unsigned int startLayer, const unsigned int endLayer,
    ClusterFitResult &clusterFitResult)
{
    try
    {
        if (startLayer >= endLayer)
            return STATUS_CODE_INVALID_PARAMETER;

        const OrderedCaloHitList &orderedCaloHitList = pCluster->GetOrderedCaloHitList();
        const unsigned int listSize(orderedCaloHitList.size());

        if (0 == listSize)
            return STATUS_CODE_NOT_INITIALIZED;

        if (listSize < 2)
            return STATUS_CODE_OUT_OF_RANGE;

        ClusterFitPointList clusterFitPointList;
        // once the hits are ordered by pseudolayer, can iterate over each layer
        // and determine the centroid of each layer, with
        // - position, cell length scale and energy = average of values of each hit
        // - direction = sum of cell directions (normal vectors), normalised to unity
        for (const OrderedCaloHitList::value_type &layerIter : orderedCaloHitList)
        {
            const unsigned int pseudoLayer(layerIter.first);

            if (startLayer > pseudoLayer)
                continue;

            if (endLayer < pseudoLayer)
                break;

            const unsigned int nCaloHits(layerIter.second->size());

            if (0 == nCaloHits)
                throw StatusCodeException(STATUS_CODE_FAILURE);

            float cellLengthScaleSum(0.f), cellEnergySum(0.f);
            CartesianVector cellNormalVectorSum(0.f, 0.f, 0.f);

            for (const CaloHit *const pCaloHit : *layerIter.second)
            {
                cellLengthScaleSum += pCaloHit->GetCellLengthScale();
                cellNormalVectorSum += pCaloHit->GetCellNormalVector();
                cellEnergySum += pCaloHit->GetInputEnergy();
            }

            clusterFitPointList.push_back(ClusterFitPoint(pCluster->GetCentroid(pseudoLayer), cellNormalVectorSum.GetUnitVector(),
                cellLengthScaleSum / static_cast<float>(nCaloHits), cellEnergySum / static_cast<float>(nCaloHits), pseudoLayer));
        }

        // then, fit the centroids rather than fitting all hits in clusters
        return FitPoints(clusterFitPointList, clusterFitResult);
    }
    catch (StatusCodeException &statusCodeException)
    {
        clusterFitResult.SetSuccessFlag(false);
        return statusCodeException.GetStatusCode();
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ClusterFitHelper::FitPoints(ClusterFitPointList &clusterFitPointList, ClusterFitResult &clusterFitResult)
{
    std::sort(clusterFitPointList.begin(), clusterFitPointList.end());

    try
    {
        const unsigned int nFitPoints(clusterFitPointList.size());
        std::cout << "Number of points for fit: " << nFitPoints << std::endl;

        if (nFitPoints < 2)
            return STATUS_CODE_INVALID_PARAMETER;

        clusterFitResult.Reset();
        CartesianVector positionSum(0.f, 0.f, 0.f);
        CartesianVector normalVectorSum(0.f, 0.f, 0.f);

        for (const ClusterFitPoint &clusterFitPoint : clusterFitPointList)
        {
            positionSum += clusterFitPoint.GetPosition();
            normalVectorSum += clusterFitPoint.GetCellNormalVector();
        }

        return PerformLinearFit(positionSum * (1.f / static_cast<float>(nFitPoints)), normalVectorSum.GetUnitVector(), clusterFitPointList, clusterFitResult);
    }
    catch (StatusCodeException &statusCodeException)
    {
        std::cout << "ClusterFitHelper: linear fit to cluster failed. " << std::endl;
        clusterFitResult.SetSuccessFlag(false);
        return statusCodeException.GetStatusCode();
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ClusterFitHelper::PerformLinearFit(const CartesianVector &centralPosition, const CartesianVector &centralDirection,
    ClusterFitPointList &clusterFitPointList, ClusterFitResult &clusterFitResult)
{
    std::cout << "Performing linear fit for cluster" << std::endl;
    std::cout << "  initial position: " << centralPosition << std::endl;
    std::cout << "  initial direction: " << centralDirection << std::endl;

    std::sort(clusterFitPointList.begin(), clusterFitPointList.end());

    // Extract the data
    double sumP(0.), sumQ(0.), sumR(0.), sumWeights(0.);
    double sumPR(0.), sumQR(0.), sumRR(0.);

    // Rotate the coordinate system to align the estimated initial direction (centralDirection)
    // with the z axis (chosenAxis) using Rodrigues rotation formula
    // Points are also translated so that the centroid (centralPosition) is at the origin
    const CartesianVector chosenAxis(0.f, 0.f, 1.f);
    const double cosTheta(centralDirection.GetCosOpeningAngle(chosenAxis));
    const double sinTheta(std::sin(std::acos(cosTheta)));

    const CartesianVector rotationAxis((std::fabs(cosTheta) > 0.99) ? CartesianVector(1.f, 0.f, 0.f) :
        centralDirection.GetCrossProduct(chosenAxis).GetUnitVector());

    for (const ClusterFitPoint &clusterFitPoint : clusterFitPointList)
    {
        const CartesianVector position(clusterFitPoint.GetPosition() - centralPosition);
        const double weight(1.);

        const double p( (cosTheta + rotationAxis.GetX() * rotationAxis.GetX() * (1. - cosTheta)) * position.GetX() +
            (rotationAxis.GetX() * rotationAxis.GetY() * (1. - cosTheta) - rotationAxis.GetZ() * sinTheta) * position.GetY() +
            (rotationAxis.GetX() * rotationAxis.GetZ() * (1. - cosTheta) + rotationAxis.GetY() * sinTheta) * position.GetZ() );
        const double q( (rotationAxis.GetY() * rotationAxis.GetX() * (1. - cosTheta) + rotationAxis.GetZ() * sinTheta) * position.GetX() +
            (cosTheta + rotationAxis.GetY() * rotationAxis.GetY() * (1. - cosTheta)) * position.GetY() +
            (rotationAxis.GetY() * rotationAxis.GetZ() * (1. - cosTheta) - rotationAxis.GetX() * sinTheta) * position.GetZ() );
        const double r( (rotationAxis.GetZ() * rotationAxis.GetX() * (1. - cosTheta) - rotationAxis.GetY() * sinTheta) * position.GetX() +
            (rotationAxis.GetZ() * rotationAxis.GetY() * (1. - cosTheta) + rotationAxis.GetX() * sinTheta) * position.GetY() +
            (cosTheta + rotationAxis.GetZ() * rotationAxis.GetZ() * (1. - cosTheta)) * position.GetZ() );

        sumP += p * weight; sumQ += q * weight; sumR += r * weight;
        sumPR += p * r * weight; sumQR += q * r * weight; sumRR += r * r * weight;
        sumWeights += weight;
    }

    // Once the points are rotated, perform a 2D linear regression in (p, q) plane as a function of r(z)
    // i.e. find best fitting lines: p = a_p*r + b_p, q = a_q*r + b_q
    const double denominatorR(sumR * sumR - sumWeights * sumRR);

    if (std::fabs(denominatorR) < std::numeric_limits<double>::epsilon()) {
        std::cout << "  fit failed" << std::endl;
        return STATUS_CODE_FAILURE;
    }

    const double aP((sumR * sumP - sumWeights * sumPR) / denominatorR);
    const double bP((sumP - aP * sumR) / sumWeights);
    const double aQ((sumR * sumQ - sumWeights * sumQR) / denominatorR);
    const double bQ((sumQ - aQ * sumR) / sumWeights);

    // Convert fitted line back to 3D

    // Extract direction in 3D: (a_p, a_q, 1) normalised to 1
    const double magnitude(std::sqrt(1. + aP * aP + aQ * aQ));
    const double dirP(aP / magnitude), dirQ(aQ / magnitude), dirR(1. / magnitude);

    // Rotate the direction and intercept back to original frame
    // Reverse rotation applied to direction vector to go back to original frame
    CartesianVector direction(
        static_cast<float>((cosTheta + rotationAxis.GetX() * rotationAxis.GetX() * (1. - cosTheta)) * dirP +
            (rotationAxis.GetX() * rotationAxis.GetY() * (1. - cosTheta) + rotationAxis.GetZ() * sinTheta) * dirQ +
            (rotationAxis.GetX() * rotationAxis.GetZ() * (1. - cosTheta) - rotationAxis.GetY() * sinTheta) * dirR),
        static_cast<float>((rotationAxis.GetY() * rotationAxis.GetX() * (1. - cosTheta) - rotationAxis.GetZ() * sinTheta) * dirP +
            (cosTheta + rotationAxis.GetY() * rotationAxis.GetY() * (1. - cosTheta)) * dirQ +
            (rotationAxis.GetY() * rotationAxis.GetZ() * (1. - cosTheta) + rotationAxis.GetX() * sinTheta) * dirR),
        static_cast<float>((rotationAxis.GetZ() * rotationAxis.GetX() * (1. - cosTheta) + rotationAxis.GetY() * sinTheta) * dirP +
            (rotationAxis.GetZ() * rotationAxis.GetY() * (1. - cosTheta) - rotationAxis.GetX() * sinTheta) * dirQ +
            (cosTheta + rotationAxis.GetZ() * rotationAxis.GetZ() * (1. - cosTheta)) * dirR) );

    // Similar transformation for intercept (which is defined as the point of th best-fit line for r=0)
    // i.e. at same z as centroid for endcap and at same rho as centroid for barrel?
    // Additional translation to shift back the centroid at the proper position
    CartesianVector intercept(centralPosition + CartesianVector(
        static_cast<float>((cosTheta + rotationAxis.GetX() * rotationAxis.GetX() * (1. - cosTheta)) * bP +
            (rotationAxis.GetX() * rotationAxis.GetY() * (1. - cosTheta) + rotationAxis.GetZ() * sinTheta) * bQ),
        static_cast<float>((rotationAxis.GetY() * rotationAxis.GetX() * (1. - cosTheta) - rotationAxis.GetZ() * sinTheta) * bP +
            (cosTheta + rotationAxis.GetY() * rotationAxis.GetY() * (1. - cosTheta)) * bQ),
        static_cast<float>((rotationAxis.GetZ() * rotationAxis.GetX() * (1. - cosTheta) + rotationAxis.GetY() * sinTheta) * bP +
            (rotationAxis.GetZ() * rotationAxis.GetY() * (1. - cosTheta) - rotationAxis.GetX() * sinTheta) * bQ) ));

    // Extract radial direction cosine: cosine of angle between fitted direction, and direction calculated from
    // intercept ("best-fit" centroid) assuming projectivity from IP
    float dirCosR(direction.GetDotProduct(intercept) / intercept.GetMagnitude());

    if (0.f > dirCosR)
    {
        dirCosR = -dirCosR;
        direction = direction * -1.f;
    }

    // Now calculate something like a chi2
    double chi2_P(0.), chi2_Q(0.), rms(0.);
    double sumA(0.), sumL(0.), sumAL(0.), sumLL(0.);

    for (const ClusterFitPoint &clusterFitPoint : clusterFitPointList)
    {
        const CartesianVector position(clusterFitPoint.GetPosition() - centralPosition);

        const double p( (cosTheta + rotationAxis.GetX() * rotationAxis.GetX() * (1. - cosTheta)) * position.GetX() +
            (rotationAxis.GetX() * rotationAxis.GetY() * (1. - cosTheta) - rotationAxis.GetZ() * sinTheta) * position.GetY() +
            (rotationAxis.GetX() * rotationAxis.GetZ() * (1. - cosTheta) + rotationAxis.GetY() * sinTheta) * position.GetZ() );
        const double q( (rotationAxis.GetY() * rotationAxis.GetX() * (1. - cosTheta) + rotationAxis.GetZ() * sinTheta) * position.GetX() +
            (cosTheta + rotationAxis.GetY() * rotationAxis.GetY() * (1. - cosTheta)) * position.GetY() +
            (rotationAxis.GetY() * rotationAxis.GetZ() * (1. - cosTheta) - rotationAxis.GetX() * sinTheta) * position.GetZ() );
        const double r( (rotationAxis.GetZ() * rotationAxis.GetX() * (1. - cosTheta) - rotationAxis.GetY() * sinTheta) * position.GetX() +
            (rotationAxis.GetZ() * rotationAxis.GetY() * (1. - cosTheta) + rotationAxis.GetX() * sinTheta) * position.GetY() +
            (cosTheta + rotationAxis.GetZ() * rotationAxis.GetZ() * (1. - cosTheta)) * position.GetZ() );

        const double error(clusterFitPoint.GetCellSize() / 3.46);
        const double chiP((p - aP * r - bP) / error);
        const double chiQ((q - aQ * r - bQ) / error);

        chi2_P += chiP * chiP;
        chi2_Q += chiQ * chiQ;

        const CartesianVector difference(clusterFitPoint.GetPosition() - intercept);
        rms += (direction.GetCrossProduct(difference)).GetMagnitudeSquared();

        const float a(direction.GetDotProduct(difference));
        const float l(static_cast<float>(clusterFitPoint.GetPseudoLayer()));
        sumA += a; sumL += l; sumAL += a * l; sumLL += l * l;
    }

    const double nPoints(static_cast<double>(clusterFitPointList.size()));
    const double denominatorL(sumL * sumL - nPoints * sumLL);

    if (std::fabs(denominatorL) > std::numeric_limits<double>::epsilon())
    {
        if (0. > ((sumL * sumA - nPoints * sumAL) / denominatorL))
            direction = direction * -1.f;
    }

    clusterFitResult.SetDirection(direction);
    clusterFitResult.SetIntercept(intercept);
    clusterFitResult.SetChi2(static_cast<float>((chi2_P + chi2_Q) / nPoints));
    clusterFitResult.SetRms(static_cast<float>(std::sqrt(rms / nPoints)));
    clusterFitResult.SetRadialDirectionCosine(dirCosR);
    clusterFitResult.SetSuccessFlag(true);

    std::cout << "  fit successful" << std::endl;
    std::cout << "  final position: " << intercept << std::endl;
    std::cout << "  final direction: " << direction << std::endl;
    std::cout << "  rms: " << clusterFitResult.GetRms() << std::endl;
    std::cout << "  cos(dRdir): " << dirCosR << std::endl;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

ClusterFitPoint::ClusterFitPoint(const CaloHit *const pCaloHit) :
    m_position(pCaloHit->GetPositionVector()),
    m_cellNormalVector(pCaloHit->GetCellNormalVector()),
    m_cellSize(pCaloHit->GetCellLengthScale()),
    m_energy(pCaloHit->GetInputEnergy()),
    m_pseudoLayer(pCaloHit->GetPseudoLayer())
{
    if (m_cellSize < std::numeric_limits<float>::epsilon())
        throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);
}

//------------------------------------------------------------------------------------------------------------------------------------------

ClusterFitPoint::ClusterFitPoint(const CartesianVector &position, const CartesianVector &cellNormalVector, const float cellSize,
        const float energy, const unsigned int pseudoLayer) :
    m_position(position),
    m_cellNormalVector(cellNormalVector.GetUnitVector()),
    m_cellSize(cellSize),
    m_energy(energy),
    m_pseudoLayer(pseudoLayer)
{
    if (m_cellSize < std::numeric_limits<float>::epsilon())
        throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);
}

//------------------------------------------------------------------------------------------------------------------------------------------

bool ClusterFitPoint::operator<(const ClusterFitPoint &rhs) const
{
    const CartesianVector deltaPosition(rhs.GetPosition() - this->GetPosition());

    if (std::fabs(deltaPosition.GetZ()) > std::numeric_limits<float>::epsilon())
        return (deltaPosition.GetZ() > std::numeric_limits<float>::epsilon());

    if (std::fabs(deltaPosition.GetX()) > std::numeric_limits<float>::epsilon())
        return (deltaPosition.GetX() > std::numeric_limits<float>::epsilon());

    if (std::fabs(deltaPosition.GetY()) > std::numeric_limits<float>::epsilon())
        return (deltaPosition.GetY() > std::numeric_limits<float>::epsilon());

    return (this->GetEnergy() > rhs.GetEnergy());
}

} // namespace pandora
