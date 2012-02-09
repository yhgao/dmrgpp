/*
Copyright (c) 2009, UT-Battelle, LLC
All rights reserved

[DMRG++, Version 2.0.0]
[by G.A., Oak Ridge National Laboratory]

UT Battelle Open Source Software License 11242008

OPEN SOURCE LICENSE

Subject to the conditions of this License, each
contributor to this software hereby grants, free of
charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), a
perpetual, worldwide, non-exclusive, no-charge,
royalty-free, irrevocable copyright license to use, copy,
modify, merge, publish, distribute, and/or sublicense
copies of the Software.

1. Redistributions of Software must retain the above
copyright and license notices, this list of conditions,
and the following disclaimer.  Changes or modifications
to, or derivative works of, the Software should be noted
with comments and the contributor and organization's
name.

2. Neither the names of UT-Battelle, LLC or the
Department of Energy nor the names of the Software
contributors may be used to endorse or promote products
derived from this software without specific prior written
permission of UT-Battelle.

3. The software and the end-user documentation included
with the redistribution, with or without modification,
must include the following acknowledgment:

"This product includes software produced by UT-Battelle,
LLC under Contract No. DE-AC05-00OR22725  with the
Department of Energy."
 
*********************************************************
DISCLAIMER

THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER, CONTRIBUTORS, UNITED STATES GOVERNMENT,
OR THE UNITED STATES DEPARTMENT OF ENERGY BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED
STATES DEPARTMENT OF ENERGY, NOR THE COPYRIGHT OWNER, NOR
ANY OF THEIR EMPLOYEES, REPRESENTS THAT THE USE OF ANY
INFORMATION, DATA, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.

*********************************************************


*/

/** \ingroup DMRG */
/*@{*/

/*! \file ReflectionTransform
 *
 *
 */
#ifndef reflectionTRANSFORM_H
#define reflectionTRANSFORM_H

#include "PackIndices.h" // in PsimagLite
#include "Matrix.h"
#include "ProgressIndicator.h"
#include "LAPACK.h"
#include "Sort.h"
#include "ReflectionBasis.h"

namespace Dmrg {

template<typename RealType,typename SparseMatrixType>
class ReflectionTransform {

	typedef typename SparseMatrixType::value_type ComplexOrRealType;
	typedef std::vector<ComplexOrRealType> VectorType;
	typedef SparseVector<typename VectorType::value_type> SparseVectorType;
	typedef ReflectionBasis<RealType,SparseMatrixType> ReflectionBasisType;

public:

	void update(const SparseMatrixType& sSector)
	{
		ReflectionBasisType reflectionBasis(sSector);
		plusSector_ = reflectionBasis.R(1.0).rank();
		computeTransform(Q1_,reflectionBasis,1.0);
		computeTransform(Qm_,reflectionBasis,-1.0);
//		printFullMatrix(Q1_,"Q1");
//		printFullMatrix(Qm_,"Qm");


	}

	void transform(SparseMatrixType& dest1,
		       SparseMatrixType& destm,
		       const SparseMatrixType& H) const
	{
		SparseMatrixType HQ1,HQm;
		multiply(HQ1,H,Q1_);

		multiply(HQm,H,Qm_);
//		printFullMatrix(HQm,"HQm");
//		printFullMatrix(HQ1,"HQ1");

		SparseMatrixType Q1t,Qmt;
		transposeConjugate(Q1t,Q1_);
		transposeConjugate(Qmt,Qm_);
		multiply(dest1,Q1t,HQ1);
		reshape(dest1,plusSector_);
		multiply(destm,Qmt,HQm);
		reshape(destm,H.rank()-plusSector_);
//		printFullMatrix(dest1,"dest1");
//		printFullMatrix(destm,"destm");

#ifndef NDEBUG
		checkTransform(Qmt,HQ1);
		checkTransform(Q1t,HQm);
#endif
	}

	void setGs(VectorType& gs,const VectorType& v,const RealType& sector) const
	{
		const SparseMatrixType& Q = (sector>0) ? Q1_ : Qm_;
		multiply(gs,Q,v);
		RealType norma = PsimagLite::norm(gs);
		gs /= norma;
	}

	template<typename SomeVectorType>
	void setInitState(const SomeVectorType& initVector,
			  SomeVectorType& initVector1,
			  SomeVectorType& initVector2) const
	{
		size_t minusSector = initVector.size()-plusSector_;
		initVector1.resize(plusSector_);
		initVector2.resize(minusSector);
		for (size_t i=0;i<initVector.size();i++) {
			if (i<plusSector_) initVector1[i]=initVector[i];
			else  initVector2[i-plusSector_]=initVector[i];
		}
	}

private:

	void reshape(SparseMatrixType& A,size_t n2) const
	{
		SparseMatrixType B(n2,n2);
		size_t counter = 0;
		for (size_t i=0;i<n2;i++) {
			B.setRow(i,counter);
			for (int k = A.getRowPtr(i);k<A.getRowPtr(i+1);k++) {
				size_t col = A.getCol(k);
				if (col>=n2) continue;
				B.pushCol(col);
				B.pushValue(A.getValue(k));
				counter++;
			}
		}
		B.setRow(n2,counter);
		B.checkValidity();
		A = B;
	}

	void checkTransform(const SparseMatrixType& A,const SparseMatrixType& B) const
	{
		SparseMatrixType C;
		multiply(C,A,B);
//		printFullMatrix(A,"MatrixA");
//		printFullMatrix(B,"MatrixB");
//		printFullMatrix(C,"MatrixC");
		assert(isZero(C));
	}

	void computeTransform(SparseMatrixType& Q1,
			      const ReflectionBasisType& reflectionBasis,
			      const RealType& sector)
	{
		const SparseMatrixType& R1 = reflectionBasis.R(sector);
		SparseMatrixType R1Inverse;
		reflectionBasis.inverseTriangular(R1Inverse,R1);
//		printFullMatrix(R1Inverse,"R1Inverse");
		SparseMatrixType T1;

		buildT1(T1,reflectionBasis,sector);
		bool strict = false; // matrices below have different ranks!!
		multiply(Q1,T1,R1Inverse,strict);
	}

	void buildT1(SparseMatrixType& T1final,
		     const ReflectionBasisType& reflectionBasis,
		     const RealType& sector) const
	{
		const std::vector<size_t>& ipPosOrNeg = reflectionBasis.ipPosOrNeg(sector);
		const SparseMatrixType& reflection = reflectionBasis.reflection();
		size_t n = reflection.rank();

		SparseMatrixType T1(n,n);
		size_t counter = 0;
		for (size_t i=0;i<n;i++) {
			T1.setRow(i,counter);
			bool hasDiagonal = false;
			for (int k = reflection.getRowPtr(i);k<reflection.getRowPtr(i+1);k++) {
				size_t col = reflection.getCol(k);
				ComplexOrRealType val = reflection.getValue(k);
				if (col==i) {
					val += sector;
					hasDiagonal=true;
				}
				val *= sector;
				T1.pushCol(col);
				T1.pushValue(val);
				counter++;
			}
			if (!hasDiagonal) {
				T1.pushCol(i);
				T1.pushValue(1.0);
				counter++;
			}
		}
		T1.setRow(n,counter);
		T1.checkValidity();

		// permute columns now:
		std::vector<int> inversePermutation(n,-1);
		for (size_t i=0;i<ipPosOrNeg.size();i++)
			inversePermutation[ipPosOrNeg[i]]=i;

		T1final.resize(n);
		counter=0;
		for (size_t i=0;i<n;i++) {
			T1final.setRow(i,counter);
			for (int k = T1.getRowPtr(i);k<T1.getRowPtr(i+1);k++) {
				int col = inversePermutation[T1.getCol(k)];
				if (col<0) continue;
				T1final.pushCol(col);
				T1final.pushValue(T1.getValue(k));
				counter++;
			}
		}
		T1final.setRow(n,counter);
		T1final.checkValidity();
	}

	//	void checkTransform(const SparseMatrixType& sSector)
	//	{
	//		SparseMatrixType rT;
	//		transposeConjugate(rT,transform_);
	//		SparseMatrixType tmp3;
	//		multiply(tmp3,transform_,rT);

	////		printFullMatrix(rT,"Transform");

	//		SparseMatrixType tmp4;
	//		multiply(tmp3,sSector,rT);
	//		multiply(tmp4,transform_,tmp3);
	////		printFullMatrix(tmp4,"R S R^\\dagger");
	//	}

//	void split(SparseMatrixType& matrixA,SparseMatrixType& matrixB,const SparseMatrixType& matrix) const
//	{
//		size_t counter = 0;
//		matrixA.resize(plusSector_);
//		for (size_t i=0;i<plusSector_;i++) {
//			matrixA.setRow(i,counter);
//			for (int k=matrix.getRowPtr(i);k<matrix.getRowPtr(i+1);k++) {
//				size_t col = matrix.getCol(k);
//				ComplexOrRealType val = matrix.getValue(k);
//				if (col<plusSector_) {
//					matrixA.pushCol(col);
//					matrixA.pushValue(val);
//					counter++;
//					continue;
//				}
//				if (!isAlmostZero(val,1e-5)) {
//					std::string s(__FILE__);
//					s += " Hamiltonian has no reflection symmetry.";
//					throw std::runtime_error(s.c_str());
//				}
//			}
//		}
//		matrixA.setRow(plusSector_,counter);

//		size_t rank = matrix.rank();
//		size_t minusSector=rank-plusSector_;
//		matrixB.resize(minusSector);
//		counter=0;
//		for (size_t i=plusSector_;i<rank;i++) {
//			matrixB.setRow(i-plusSector_,counter);
//			for (int k=matrix.getRowPtr(i);k<matrix.getRowPtr(i+1);k++) {
//				size_t col = matrix.getCol(k);
//				ComplexOrRealType val = matrix.getValue(k);
//				if (col>=plusSector_) {
//					matrixB.pushCol(col-plusSector_);
//					matrixB.pushValue(val);
//					counter++;
//					continue;
//				}
//				if (!isAlmostZero(val,1e-5)) {
//					std::string s(__FILE__);
//					s += " Hamiltonian has no reflection symmetry.";
//					throw std::runtime_error(s.c_str());
//				}
//			}
//		}
//		matrixB.setRow(minusSector,counter);
//	}

	size_t plusSector_;
	SparseMatrixType Q1_,Qm_;

}; // class ReflectionTransform

} // namespace Dmrg 

/*@}*/
#endif // reflectionTRANSFORM_H
