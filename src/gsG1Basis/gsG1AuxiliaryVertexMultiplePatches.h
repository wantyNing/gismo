//
// Created by afarahat on 2/25/20.
//

#pragma once

#include <gismo.h>
#include <gsCore/gsMultiPatch.h>
#include <gsG1Basis/gsG1AuxiliaryPatch.h>

# include <gsG1Basis/ApproxG1Basis/gsApproxG1BasisVertex.h>
# include <gsG1Basis/gsG1ASBasisVertex.h>

# include <gsG1Basis/gsG1OptionList.h>

namespace gismo
{

class gsG1AuxiliaryVertexMultiplePatches
{

public:

// Constructor for n patches around a common vertex
    gsG1AuxiliaryVertexMultiplePatches(const gsMultiPatch<> & mp, const std::vector<size_t> patchesAroundVertex, const std::vector<size_t> vertexIndices)
    {
        for(size_t i = 0; i < patchesAroundVertex.size(); i++)
        {
            auxGeom.push_back(gsG1AuxiliaryPatch(mp.patch(patchesAroundVertex[i]), patchesAroundVertex[i]));
            auxVertexIndices.push_back(vertexIndices[i]);
            checkBoundary(mp, patchesAroundVertex[i], vertexIndices[i]);
        }
//        gsInfo <<  patchesAroundVertex.size() << " patch constructed \n";
        sigma = 0.0;
    }


    // Compute topology
    // After computeTopology() the patches will have the same patch-index as the position-index in auxGeom
    // EXAMPLE: global patch-index-order inside auxGeom: [2, 3, 4, 1, 0]
    //          in auxTop: 2->0, 3->1, 4->2, 1->3, 0->4
    gsMultiPatch<> computeAuxTopology(){
        gsMultiPatch<> auxTop;
        for(unsigned i = 0; i <  auxGeom.size(); i++)
        {
            auxTop.addPatch(auxGeom[i].getPatch());
        }
        auxTop.computeTopology();
        return auxTop;
    }


    void reparametrizeG1Vertex()
    {
        for(size_t i = 0; i < auxGeom.size(); i++)
        {
            checkOrientation(i); // Check if the orientation is correct. If not, modifies vertex and edge vectors

            switch (auxVertexIndices[i])
            {
                case 1:
//                    gsInfo << "Patch: " << auxGeom[i].getGlobalPatchIndex() << " not rotated\n";
                    break;
                case 4:
                    auxGeom[i].rotateParamAntiClockTwice();
//                    gsInfo << "Patch: " << auxGeom[i].getGlobalPatchIndex() << " rotated twice anticlockwise\n";
                    break;
                case 2:
                    auxGeom[i].rotateParamAntiClock();
//                    gsInfo << "Patch: " << auxGeom[i].getGlobalPatchIndex() << " rotated anticlockwise\n";
                    break;
                case 3:
                    auxGeom[i].rotateParamClock();
//                    gsInfo << "Patch: " << auxGeom[i].getGlobalPatchIndex() << " rotated clockwise\n";
                    break;
            }
        }
    }


    index_t kindOfVertex()
    {
        if(auxGeom.size() == 1)
            return -1; // Boundary vertex

        gsMultiPatch<> top(computeAuxTopology());
        size_t nInt = top.interfaces().size();
        if(auxGeom.size() == nInt)
            return 0; // Internal vertex
        else
            return 1; // Interface-Boundary vertex
    }


    void checkOrientation(size_t i)
    {
        if (auxGeom[i].getPatch().orientation() == -1)
        {
            auxGeom[i].swapAxis();
//            gsInfo << "Changed axis on patch: " << auxGeom[i].getGlobalPatchIndex() << "\n";

            this->swapBdy(i); //Swap boundary edge bool-value

            // Swap vertices index after swapping axis
            if(auxVertexIndices[i] == 2)
                auxVertexIndices[i] = 3;
            else
            if(auxVertexIndices[i] == 3)
                auxVertexIndices[i] = 2;
        }
    }


    void computeSigma()
    {
        real_t p = 0;
        real_t h_geo = 0;
        for(size_t i = 0; i < auxGeom.size(); i++)
        {
            gsTensorBSplineBasis<2, real_t> & bsp_temp = dynamic_cast<gsTensorBSplineBasis<2, real_t> & >(auxGeom[i].getPatch().basis());
            real_t p_temp = bsp_temp.maxDegree();
            p = (p < p_temp ? p_temp : p);

            for(index_t j = 0; j < auxGeom[i].getPatch().parDim(); j++)
            {
               real_t h_geo_temp = bsp_temp.component(j).knots().at(p + 2);
               h_geo = (h_geo < h_geo_temp ? h_geo_temp : h_geo);
            }
        }
        real_t val = auxGeom.size();

        gsMatrix<> zero;
        zero.setZero(2,1);
        for (index_t i = 0; i < val; i++)
            sigma += auxGeom[i].getPatch().deriv(zero).lpNorm<Eigen::Infinity>();
        sigma *= h_geo/(val*p);
        sigma = 1 / sigma;


    }


    void checkBoundary(const gsMultiPatch<> & mpTmp, size_t  patchInd, size_t sideInd)
    {
        std::vector<bool> tmp;
        switch (sideInd)
        {
            case 1: tmp.push_back(mpTmp.isBoundary(patchInd,3));
                    tmp.push_back(mpTmp.isBoundary(patchInd,1));
//                    gsInfo << "Edge 3: " << mpTmp.isBoundary(patchInd, 3) << "\t Edge 1: " << mpTmp.isBoundary(patchInd, 1) << "\n";
                break;
            case 2: tmp.push_back(mpTmp.isBoundary(patchInd, 2));
                    tmp.push_back(mpTmp.isBoundary(patchInd, 3));
//                    gsInfo << "Edge 2: " << mpTmp.isBoundary(patchInd, 2) << "\t Edge 3: " << mpTmp.isBoundary(patchInd, 3) << "\n";
                break;
            case 3: tmp.push_back(mpTmp.isBoundary(patchInd, 1));
                    tmp.push_back(mpTmp.isBoundary(patchInd, 4));
//                    gsInfo << "Edge 1: " << mpTmp.isBoundary(patchInd, 1) << "\t Edge 4: " << mpTmp.isBoundary(patchInd, 4) << "\n";
                break;
            case 4: tmp.push_back(mpTmp.isBoundary(patchInd, 4));
                    tmp.push_back(mpTmp.isBoundary(patchInd, 2));
//                    gsInfo << "Edge 4: " << mpTmp.isBoundary(patchInd, 4) << "\t Edge 2: " << mpTmp.isBoundary(patchInd, 2) << "\n";
                break;
            default:
                break;
        }
        isBdy.push_back(tmp);
    }


    void swapBdy(size_t i)
    {
        bool tmp = isBdy[i][0];
        isBdy[i][0] = isBdy[i][1];
        isBdy[i][1] = tmp;
    }


    gsMatrix<> computeBigSystemMatrix( index_t np)
    {
        gsMultiBasis<> bas(auxGeom[np].getPatch());
        gsTensorBSplineBasis<2, real_t> & temp_L = dynamic_cast<gsTensorBSplineBasis<2, real_t> &>(bas.basis(0));
        size_t dimU = temp_L.size(0);
        size_t dimV = temp_L.size(1);

        gsMatrix<> BigMatrix;
        BigMatrix.setZero( 2 * (dimU + dimV - 2),auxGeom[np].getG1Basis().nPatches());

        for(size_t bf = 0; bf < auxGeom[np].getG1Basis().nPatches(); bf++)
        {
            for (size_t i = 0; i < 2 * dimU; i++)
            {
                if (auxGeom[np].getG1BasisCoefs(bf).at(i) * auxGeom[np].getG1BasisCoefs(bf).at(i) > m_zero*m_zero)
                    BigMatrix(i, bf) = auxGeom[np].getG1BasisCoefs(bf).at(i);
            }

            for (size_t i = 1; i < dimV - 1; i++)
            {
                for(size_t j = i; j < i + 2; j++)
                {
                    if (auxGeom[np].getG1BasisCoefs(bf).at((i + 1) * dimU + j - i) * auxGeom[np].getG1BasisCoefs(bf).at((i + 1) * dimU + j - i) > m_zero*m_zero)
                        BigMatrix(i + j + (2 * dimU ) - 2, bf) = auxGeom[np].getG1BasisCoefs(bf).at((i + 1) * dimU + j - i);
                }
            }
        }
        return BigMatrix;
    }


    gsMatrix<> computeSmallSystemMatrix( index_t np)
    {
        gsMultiBasis<> bas(auxGeom[np].getPatch());
        gsTensorBSplineBasis<2, real_t> & temp_L = dynamic_cast<gsTensorBSplineBasis<2, real_t> &>(bas.basis(0));
        size_t dimU = temp_L.size(0);
        size_t dimV = temp_L.size(1);

        gsMatrix<> SmallMatrix;
        SmallMatrix.setZero((dimU + dimV - 1),auxGeom[np].getG1Basis().nPatches());

        for(size_t bf = 0; bf < auxGeom[np].getG1Basis().nPatches(); bf++)
        {
            for (size_t i = 0; i < dimU; i++)
            {
                if (auxGeom[np].getG1BasisCoefs(bf).at(i) * auxGeom[np].getG1BasisCoefs(bf).at(i) > m_zero*m_zero)
                    SmallMatrix(i, bf) = auxGeom[np].getG1BasisCoefs(bf).at(i);
            }

            for (size_t i = 1; i < dimV; i++)
            {
                if (auxGeom[np].getG1BasisCoefs(bf).at(i * dimU) * auxGeom[np].getG1BasisCoefs(bf).at(i * dimU) > m_zero*m_zero)
                    SmallMatrix(i + dimU -1, bf) = auxGeom[np].getG1BasisCoefs(bf).at(i * dimU);
            }
        }
        return SmallMatrix;
    }


    gsMatrix<> leftBoundaryBigSystem(index_t np)
    {
        gsMultiBasis<> bas(auxGeom[np].getPatch());
        gsTensorBSplineBasis<2, real_t> & temp_L = dynamic_cast<gsTensorBSplineBasis<2, real_t> &>(bas.basis(0));
        size_t dimU = temp_L.size(0);
        size_t dimV = temp_L.size(1);

        gsMatrix<> BigMatrix;
        BigMatrix.setZero( 2 * dimV,6);

        for(size_t bf = 0; bf < 6; bf++)
        {
            for (size_t i = 0; i < dimV ; i++)
            {
                for(size_t j = i; j < i + 2; j++)
                {
                    if (auxGeom[np].getG1BasisCoefs(bf).at(i * dimU + j - i) * auxGeom[np].getG1BasisCoefs(bf).at(i * dimU + j - i) > m_zero*m_zero)
                        BigMatrix(i + j, bf) = auxGeom[np].getG1BasisCoefs(bf).at(i  * dimU + j - i);
                    else
                        auxGeom[np].getG1BasisCoefs(bf).at(i) *= 0;
                }
            }
        }
        return BigMatrix;
    }


    gsMatrix<> leftBoundarySmallSystem( index_t np)
    {
        gsMultiBasis<> bas(auxGeom[np].getPatch());
        gsTensorBSplineBasis<2, real_t> & temp_L = dynamic_cast<gsTensorBSplineBasis<2, real_t> &>(bas.basis(0));
        size_t dimU = temp_L.size(0);
        size_t dimV = temp_L.size(1);

        gsMatrix<> SmallMatrix;
        SmallMatrix.setZero(dimV,6);

        for(size_t bf = 0; bf < 6; bf++)
        {
            for (size_t i = 0; i < dimV; i++)
            {
                if (auxGeom[np].getG1BasisCoefs(bf).at(i * dimU) * auxGeom[np].getG1BasisCoefs(bf).at(i * dimU) > m_zero*m_zero)
                    SmallMatrix(i, bf) = auxGeom[np].getG1BasisCoefs(bf).at(i * dimU);
                else
                    auxGeom[np].getG1BasisCoefs(bf).at(i * dimU) *= 0;
            }
        }
        return SmallMatrix;
    }


    gsMatrix<> rightBoundaryBigSystem( index_t np)
    {
        gsMultiBasis<> bas(auxGeom[np].getPatch());
        gsTensorBSplineBasis<2, real_t> & temp_L = dynamic_cast<gsTensorBSplineBasis<2, real_t> &>(bas.basis(0));
        size_t dimU = temp_L.size(0);

        gsMatrix<> BigMatrix;
        BigMatrix.setZero( 2 * dimU ,6);

        for(size_t bf = 0; bf < 6; bf++)
        {
            for (size_t i = 0; i < 2 * dimU; i++)
            {
                if (auxGeom[np].getG1BasisCoefs(bf).at(i) * auxGeom[np].getG1BasisCoefs(bf).at(i) > m_zero*m_zero)
                    BigMatrix(i, bf) = auxGeom[np].getG1BasisCoefs(bf).at(i);
                else
                    auxGeom[np].getG1BasisCoefs(bf).at(i) *= 0;
            }
        }
        return BigMatrix;
    }


    gsMatrix<> rightBoundarySmallSystem( index_t np)
    {
        gsMultiBasis<> bas(auxGeom[np].getPatch());
        gsTensorBSplineBasis<2, real_t> & temp_L = dynamic_cast<gsTensorBSplineBasis<2, real_t> &>(bas.basis(0));
        //size_t dimU = temp_L.size(0);
        size_t dimU = temp_L.size(0);


        gsMatrix<> SmallMatrix;
        SmallMatrix.setZero( dimU, 6);

        for(size_t bf = 0; bf < 6; bf++)
        {
            for (size_t i = 0; i < dimU; i++)
            {
                if (auxGeom[np].getG1BasisCoefs(bf).at(i ) * auxGeom[np].getG1BasisCoefs(bf).at(i ) > m_zero*m_zero)
                    SmallMatrix(i, bf) = auxGeom[np].getG1BasisCoefs(bf).at(i);
                else
                    auxGeom[np].getG1BasisCoefs(bf).at(i) *= 0;
            }
        }
        return SmallMatrix;
    }


    gsMatrix<> bigInternalBoundaryPatchSystem( index_t np)
    {
        gsMultiBasis<> bas(auxGeom[np].getPatch());
        gsTensorBSplineBasis<2, real_t> & temp_L = dynamic_cast<gsTensorBSplineBasis<2, real_t> &>(bas.basis(0));
        size_t dimU = temp_L.size(0);

        gsMatrix<> Matrix;
        Matrix.setZero( 3 ,6);

        for(size_t bf = 0; bf < 6; bf++)
        {
            Matrix(0, bf) = auxGeom[np].getG1BasisCoefs(bf).at(0);
            Matrix(1, bf) = auxGeom[np].getG1BasisCoefs(bf).at(1);
            Matrix(2, bf) = auxGeom[np].getG1BasisCoefs(bf).at(dimU);

        }
        return Matrix;
    }


    gsMatrix<> smallInternalBoundaryPatchSystem( index_t np)
    {
        gsMatrix<> Matrix;
        Matrix.setZero( 1 ,6);

        for(size_t bf = 0; bf < 6; bf++)
        {
            Matrix(0, bf) = auxGeom[np].getG1BasisCoefs(bf).at(0);
        }
        return Matrix;
    }


    std::pair<gsMatrix<>, gsMatrix<>> createSinglePatchSystem(index_t np)
    {
        if(isBdy[np][1] == 1)
            return std::make_pair(leftBoundaryBigSystem(np), leftBoundarySmallSystem(np));
        else
        {
        if (isBdy[np][0] == 1)
            return std::make_pair(rightBoundaryBigSystem(np), rightBoundarySmallSystem(np));
        else
            return std::make_pair(bigInternalBoundaryPatchSystem(np), smallInternalBoundaryPatchSystem(np));
        }
    }


    void checkValues(gsMatrix<> & mat)
    {
        for(index_t bk = 0; bk < mat.cols(); bk++ )
        {
            for(index_t r=0; r < mat.rows(); r++)
            {
                if( abs( mat(r, bk) * mat(r, bk) )   < (m_zero*m_zero) )
                    mat(r, bk) = 0;
            }
        }
    }


    void addVertexBasis(gsMatrix<> & basisV)
    {
        gsMatrix<> vertBas;
        vertBas.setIdentity(auxGeom[0].getG1Basis().nPatches(), auxGeom[0].getG1Basis().nPatches());
        size_t count = 0;
        index_t numBF = auxGeom[0].getG1Basis().nPatches();
        while (basisV.cols() < numBF)
        {
            basisV.conservativeResize(basisV.rows(), basisV.cols() + 1);
            basisV.col(basisV.cols() - 1) = vertBas.col(count);

            Eigen::FullPivLU<gsMatrix<>> ker(basisV);
            ker.setThreshold(m_g1OptionList.getReal("threshold"));
            if (ker.dimensionOfKernel() != 0)
            {
                basisV = basisV.block(0, 0, basisV.rows(), basisV.cols() - 1);
            }
            count++;
        }
    }


    void addSmallKerBasis(gsMatrix<> & basisV, gsMatrix<> & smallK, index_t smallKDim)
    {
        for(index_t i=0; i < smallKDim; i++)
        {
            basisV.conservativeResize(basisV.rows(), basisV.cols() + 1);
            basisV.col(basisV.cols()-1) = smallK.col(i);

            Eigen::FullPivLU<gsMatrix<>> ker(basisV);
            ker.setThreshold(m_g1OptionList.getReal("threshold"));
            if(ker.dimensionOfKernel() != 0)
            {
                basisV = basisV.block(0, 0, basisV.rows(), basisV.cols()-1);
            }
        }
    }


    std::pair<gsMatrix<>, std::vector<index_t>> selectVertexBoundaryBasisFunction(gsMatrix<> bigKernel, index_t bigKerDim, gsMatrix<> smallKernel, index_t smallKerDim)
    {
        gsMatrix<> basisVect;
        std::vector<index_t> numberPerType;

        numberPerType.push_back(bigKerDim); // Number of basis which has to be moved to the internal
        numberPerType.push_back(smallKerDim - bigKerDim); // Number of basis which are boundary function of FIRST TYPE
        numberPerType.push_back(bigKernel.cols() - smallKerDim); // Number of basis which are boundary function of SECOND TYPE

        if(bigKerDim != 0)
        {
            checkValues(bigKernel);
            checkValues(smallKernel);

            basisVect = bigKernel;
            addSmallKerBasis(basisVect, smallKernel, smallKerDim);
            addVertexBasis(basisVect);
        }
        else
        {
            if (smallKerDim != 0)
            {
                checkValues(smallKernel);

                basisVect = smallKernel;
                addVertexBasis(basisVect);
            }
            else
            {
                gsMatrix<> vertBas;
                vertBas.setIdentity(auxGeom[0].getG1Basis().nPatches(), auxGeom[0].getG1Basis().nPatches());
                basisVect = vertBas;
            }

        }

//        gsInfo << "Big kernel:\n";
//        gsInfo << bigKernel << "\n ";
//
//        gsInfo << "Small kernel:\n";
//        gsInfo << smallKernel << "\n ";
//
//        gsInfo << "Basis:\n";
//        gsInfo << basisVect << "\n";

        return std::make_pair(basisVect, numberPerType);
    }


    gsMatrix<> selectGD(size_t i)
    {
        gsMatrix<> coefs(4, 2);

        if( kindOfVertex() == 1 ) // If the boundary it´s along u and along v there is an interface (Right Patch) or viceversa
        {
            gsMultiPatch<> tmp(this->computeAuxTopology());
            for(auto iter : tmp.interfaces())
            {
                if( i == iter.first().patch || i == iter.second().patch )
                {
                    gsMultiPatch<> aux;
                    if( iter.first().index() == 1 )
                    {
                        aux.addPatch(tmp.patch(iter.first().patch));
                        aux.addPatch(tmp.patch(iter.second().patch));
                    }
                    else
                    {
                        aux.addPatch(tmp.patch(iter.second().patch));
                        aux.addPatch(tmp.patch(iter.first().patch));
                    }

                    aux.computeTopology();
                    gsMultiBasis<> auxB(aux);
                    gsG1ASGluingData<real_t> ret(aux, auxB);
                    gsMatrix<> sol = ret.getSol();
                    gsMatrix<> solBeta = ret.getSolBeta();

                    if( (isBdy[i][0] == 0) && (isBdy[i][1] == 0))
                    {
                        if ( (i == iter.first().patch && iter.first().index() == 3)
                            || (i == iter.second().patch && iter.second().index() == 3) )
                        {
                            coefs(0, 0) = sol(2, 0);
                            coefs(1, 0) = sol(3, 0);
                            coefs(2, 0) = solBeta(2, 0);
                            coefs(3, 0) = solBeta(3, 0);
                        }
                        else
                        if ( (i == iter.first().patch && iter.first().index() == 1)
                            || (i == iter.second().patch && iter.second().index() == 1))
                        {
                            coefs(0, 1) = sol(0, 0);
                            coefs(1, 1) = sol(1, 0);
                            coefs(2, 1) = solBeta(0, 0);
                            coefs(3, 1) = solBeta(1, 0);
                        }
                    }
                    else
                    {
                        if ((i == iter.first().patch && iter.first().index() == 3)
                            || (i == iter.second().patch && iter.second().index() == 3))
                        {
                            coefs(0, 0) = sol(2, 0);
                            coefs(0, 1) = 1;
                            coefs(1, 0) = sol(3, 0);
                            coefs(1, 1) = 1;
                            coefs(2, 0) = solBeta(2, 0);
                            coefs(2, 1) = 0;
                            coefs(3, 0) = solBeta(3, 0);
                            coefs(3, 1) = 0;
                        }
                        else if ((i == iter.first().patch && iter.first().index() == 1)
                            || (i == iter.second().patch && iter.second().index() == 1))
                        {
                            coefs(0, 0) = 1;
                            coefs(0, 1) = sol(0, 0);
                            coefs(1, 0) = 1;
                            coefs(1, 1) = sol(1, 0);
                            coefs(2, 0) = 0;
                            coefs(2, 1) = solBeta(0, 0);
                            coefs(3, 0) = 0;
                            coefs(3, 1) = solBeta(1, 0);
                        }
                    }
                }
            }
//            gsInfo << "Coeffs boundary interface:" << coefs << "\n";
        }

        else
        if( kindOfVertex() == -1 ) // Single patch corner
        {
            coefs.setZero();
            coefs(0, 0) = 1;
            coefs(0, 1) = 1;
            coefs(1, 0) = 1;
            coefs(1, 1) = 1;
//            gsInfo << "Coeffs boundary corner:" << coefs << "\n";
        }

        else
        if( kindOfVertex() == 0 ) // Internal vertex -> Two interfaces
        {
            gsMultiPatch<> tmp(this->computeAuxTopology());
            for(auto iter : tmp.interfaces())
            {
                if( (i == iter.first().patch) || (i == iter.second().patch) )
                {
                    gsMultiPatch<> aux;
                    if( iter.first().index() == 1 )
                    {
                        aux.addPatch(tmp.patch(iter.first().patch));
                        aux.addPatch(tmp.patch(iter.second().patch));
                    }
                    else
                    {
                        aux.addPatch(tmp.patch(iter.second().patch));
                        aux.addPatch(tmp.patch(iter.first().patch));
                    }

                    aux.computeTopology();
                    gsMultiBasis<> auxB(aux);
                    gsG1ASGluingData<real_t> ret(aux, auxB);
                    gsMatrix<> sol = ret.getSol();
                    gsMatrix<> solBeta = ret.getSolBeta();

                    if ( (i == iter.first().patch && iter.first().index() == 3)
                        || (i == iter.second().patch && iter.second().index() == 3) )
                    {
                        coefs(0, 0) = sol(2, 0);
                        coefs(1, 0) = sol(3, 0);
                        coefs(2, 0) = solBeta(2, 0);
                        coefs(3, 0) = solBeta(3, 0);
                    }
                    else
                    if ( (i == iter.first().patch && iter.first().index() == 1)
                        || (i == iter.second().patch && iter.second().index() == 1))
                    {
                        coefs(0, 1) = sol(0, 0);
                        coefs(1, 1) = sol(1, 0);
                        coefs(2, 1) = solBeta(0, 0);
                        coefs(3, 1) = solBeta(1, 0);
                    }
                }
            }
//            gsInfo << "Coeffs internal:" << coefs << "\n";
        }
        return coefs;
    }



    void computeG1InternalVertexBasis(gsG1OptionList g1OptionList)
    {
        m_g1OptionList = g1OptionList;

        m_zero = g1OptionList.getReal("zero");

        this->reparametrizeG1Vertex();
        this->computeSigma();

        std::vector<gsMultiPatch<>> g1BasisVector;
        std::pair<gsMatrix<>, std::vector<index_t>> vertexBoundaryBasis;

        if(g1OptionList.getInt("user") == user::pascal)
        {
            //g1OptionList.setInt("gluingData",gluingData::global);
            //g1OptionList.setInt("p_tilde",2);

            if (g1OptionList.getSwitch("twoPatch"))
            {
                gsMultiPatch<> test_mp(auxGeom[0].getPatch());
                gsMultiBasis<> test_mb(auxGeom[0].getPatch().basis());

                gsMultiPatch<> g1Basis;
                gsBSplineBasis<> basis_edge = dynamic_cast<gsBSplineBasis<> &>(test_mp.basis(0).component(1)); // 0 -> u, 1 -> v

                for (index_t j = 0; j < 2; j++) // u
                {
                    for (index_t i = 0; i < 2; i++) // v
                    {
                        gsMatrix<> coefs;
                        coefs.setZero(test_mb.basis(0).size(),1);

                        coefs(j*(test_mb.basis(0).size()/basis_edge.size()) + i,0) = 1;

                        g1Basis.addPatch(test_mb.basis(0).makeGeometry(coefs));
                    }
                }

                g1BasisVector.push_back(g1Basis);
                auxGeom[0].setG1Basis(g1Basis);
            }
            else
            {
                for(size_t i = 0; i < auxGeom.size(); i++)
                {
                    gsMultiPatch<> g1Basis;

                    gsApproxG1BasisVertex<real_t> g1BasisVertex_0
                        (auxGeom[i].getPatch(), auxGeom[i].getPatch().basis(), isBdy[i], sigma, g1OptionList);

                    g1BasisVertex_0.setG1BasisVertex(g1Basis, this->kindOfVertex());

                    g1BasisVector.push_back(g1Basis);
                    auxGeom[i].setG1Basis(g1Basis);

//                    if (this->kindOfVertex() == 1)
//                    {
//                        gsInfo << "coefs: " << g1Basis.patch(5).coefs().reshape(auxGeom[i].getPatch().basis().component(0).size(), auxGeom[i].getPatch().basis().component(1).size()) << "\n";
//                    }
                }
            }
        }
        else if(g1OptionList.getInt("user") == user::andrea)
        {

            for(size_t i = 0; i < auxGeom.size(); i++)
            {
                gsMatrix<> gdCoefs(selectGD(i));
                gsMultiPatch<> g1Basis;

                gsG1ASBasisVertex<real_t> g1BasisVertex_0
                    (auxGeom[i].getPatch(), auxGeom[i].getPatch().basis(), isBdy[i], sigma, g1OptionList, gdCoefs);

                g1BasisVertex_0.setG1BasisVertex(g1Basis, this->kindOfVertex());

                g1BasisVector.push_back(g1Basis);
                auxGeom[i].setG1Basis(g1Basis);
            }
        }


        if (this->kindOfVertex() == 1) // Interface-Boundary vertex
        {
            gsMatrix<> bigMatrix(0,0);
            gsMatrix<> smallMatrix(0,0);
            for (size_t i = 0; i < auxGeom.size(); i++)
            {
                std::pair<gsMatrix<>, gsMatrix<>> tmp;
                tmp = createSinglePatchSystem(i);
                size_t row_bigMatrix = bigMatrix.rows();
                size_t row_smallMatrix = smallMatrix.rows();

                bigMatrix.conservativeResize(bigMatrix.rows() + tmp.first.rows(), 6);
                smallMatrix.conservativeResize(smallMatrix.rows() + tmp.second.rows(), 6);

                bigMatrix.block(row_bigMatrix, 0, tmp.first.rows(), 6) = tmp.first;
                smallMatrix.block(row_smallMatrix, 0, tmp.second.rows(), 6) = tmp.second;
            }

            Eigen::FullPivLU<gsMatrix<>> BigLU(bigMatrix);
            Eigen::FullPivLU<gsMatrix<>> SmallLU(smallMatrix);
            SmallLU.setThreshold(g1OptionList.getReal("threshold"));
            BigLU.setThreshold(g1OptionList.getReal("threshold"));

            if (!g1OptionList.getSwitch("neumann"))
                dim_kernel = SmallLU.dimensionOfKernel();
            else
                dim_kernel = BigLU.dimensionOfKernel();

            vertexBoundaryBasis = selectVertexBoundaryBasisFunction(BigLU.kernel(), BigLU.dimensionOfKernel(), SmallLU.kernel(), SmallLU.dimensionOfKernel());

        }
        else if(this->kindOfVertex() == -1) // Boundary vertex
        {
            Eigen::FullPivLU<gsMatrix<>> BigLU(computeBigSystemMatrix(0));
            Eigen::FullPivLU<gsMatrix<>> SmallLU(computeSmallSystemMatrix(0));
            SmallLU.setThreshold(g1OptionList.getReal("threshold"));
            BigLU.setThreshold(g1OptionList.getReal("threshold"));

            if (!g1OptionList.getSwitch("neumann"))
                dim_kernel = SmallLU.dimensionOfKernel();
            else
                dim_kernel = BigLU.dimensionOfKernel();

            vertexBoundaryBasis = selectVertexBoundaryBasisFunction(BigLU.kernel(), BigLU.dimensionOfKernel(), SmallLU.kernel(), SmallLU.dimensionOfKernel());

        }


        if (this->kindOfVertex() != 0)
            for (size_t i = 0; i < auxGeom.size(); i++)
            {
                gsMultiPatch<> temp_mp_g1 = g1BasisVector[i];
                for (size_t bf = 0; bf < temp_mp_g1.nPatches(); bf++)
                {
                    gsInfo << "coeffbf: " << temp_mp_g1.patch(bf).coefs().transpose() << "\n";
                    gsMatrix<> coef_bf;
                    coef_bf.setZero(temp_mp_g1.patch(bf).coefs().dim().first,1);
                    for (size_t lambda = 0; lambda < temp_mp_g1.nPatches(); lambda++)
                        coef_bf += temp_mp_g1.patch(lambda).coefs() * vertexBoundaryBasis.first(lambda,bf);
/*
                    for (index_t ii = 0; ii < coef_bf.size(); ii++)
                        if (coef_bf.at(ii) * coef_bf.at(ii) < 1e-8)
                            coef_bf.at(ii) *= 0;
*/

                    g1BasisVector[i].patch(bf).setCoefs(coef_bf);
                }

                auxGeom[i].parametrizeBasisBack(g1BasisVector[i]);
            }
        else
            for (size_t i = 0; i < auxGeom.size(); i++)
                auxGeom[i].parametrizeBasisBack(g1BasisVector[i]);

    }

    gsG1AuxiliaryPatch & getSinglePatch(const unsigned i){
        return auxGeom[i];
    }

    size_t get_internalDofs() { return dim_kernel; }


protected:
    std::vector<gsG1AuxiliaryPatch> auxGeom;
    std::vector<size_t> auxVertexIndices;
    std::vector< std::vector<bool>> isBdy;
    real_t sigma;
    size_t dim_kernel;

    real_t m_zero;

    gsG1OptionList m_g1OptionList;


};

}