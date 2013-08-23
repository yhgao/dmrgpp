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

/*! \file Operator.h
 *
 *  A structure to represent an operator
 *  Contains the actual data, the (J,M) that indicates
 * how this operator transforms, the fermionSign which
 * indicates if this operator commutes or anticommutes
 * with operators of the same class on different sites, and
 * other properties.
 *
 */
#ifndef OPERATOR_H
#define OPERATOR_H
#include "CrsMatrix.h"

namespace Dmrg {
	//! This is a structure, don't add member functions here!
	struct Su2Related {
		Su2Related()
		: offset(0) // setting to zero is necessary, because
		{}		// we always print offset
				// and when running Abelian
				// it might be undefined
		SizeType offset;
		PsimagLite::Vector<SizeType>::Type source;
		PsimagLite::Vector<int>::Type transpose;
	};

	inline std::istream& operator>>(std::istream& is,Su2Related& x)
	{
		is>>x.offset;
		return is;
	}

	inline std::ostream& operator<<(std::ostream& os,const Su2Related& x)
	{
		os<<x.offset<<"\n";
		return os;
	}

	void send(Su2Related& su2Related,int root,int tag,PsimagLite::MPI::CommType mpiComm)
	{
		PsimagLite::MPI::send(su2Related.offset,root,tag,mpiComm);
		PsimagLite::MPI::send(su2Related.source,root,tag+1,mpiComm);
		PsimagLite::MPI::send(su2Related.transpose,root,tag+2,mpiComm);
	}

	void recv(Su2Related& su2Related,int root,int tag,PsimagLite::MPI::CommType mpiComm)
	{
		PsimagLite::MPI::recv(su2Related.offset,root,tag,mpiComm);
		PsimagLite::MPI::recv(su2Related.source,root,tag+1,mpiComm);
		PsimagLite::MPI::recv(su2Related.transpose,root,tag+2,mpiComm);
	}
	

	void bcast(Su2Related& su2Related)
	{
		PsimagLite::MPI::bcast(su2Related.offset);
		PsimagLite::MPI::bcast(su2Related.source);
		PsimagLite::MPI::bcast(su2Related.transpose);
	}

	//! This is a structure, don't add member functions here!
	template<typename SparseMatrixType_>
	struct Operator {

		enum {CAN_BE_ZERO = false, MUST_BE_NONZERO = true};

		typedef PsimagLite::Vector<SizeType>::Type VectorSizeType;
		typedef SparseMatrixType_ SparseMatrixType;
		typedef typename SparseMatrixType::value_type SparseElementType;
		typedef typename PsimagLite::Real<SparseElementType>::Type RealType;
		typedef std::pair<SizeType,SizeType> PairType;
		typedef Su2Related Su2RelatedType;

		Operator() {}

		Operator(const SparseMatrixType& data1,
		         int fermionSign1,
		         const PairType& jm1,
		         RealType angularFactor1,
		         const Su2RelatedType& su2Related1)
		: data(data1),fermionSign(fermionSign1),jm(jm1),angularFactor(angularFactor1),su2Related(su2Related1)
		{}

		template<typename IoInputType, typename CookedDataType>
		Operator(IoInputType& io, CookedDataType& cookedOperator,bool checkNonZero)
		{
			PsimagLite::String s;
			PsimagLite::Matrix<SparseElementType> m;

			io.readline(s,"TSPOperator=");

			if (s == "cooked") {
				io.readline(s,"COOKED_OPERATOR=");
				VectorSizeType v;
				io.read(v,"COOKED_EXTRA");
				cookedOperator(m,s,v);
			} else {
				io.readMatrix(m,"RAW_MATRIX");
				if (checkNonZero) checkNotZeroMatrix(m);
			}

			fullMatrixToCrsMatrix(data,m);

			io.readline(fermionSign,"FERMIONSIGN=");

			VectorSizeType v(2);
			io.readKnownSize(v,"JMVALUES");
			jm.first = v[0]; jm.second = v[1];

			io.readline(angularFactor,"AngularFactor=");

			// FIXME: su2related needs to be set properly for when SU(2) is running
		}

		void send(int root,int tag,PsimagLite::MPI::CommType mpiComm)
		{
			data.send(root,tag,mpiComm);
			PsimagLite::MPI::send(fermionSign,root,tag+1,mpiComm);
			PsimagLite::MPI::send(jm,root,tag+2,mpiComm);
			PsimagLite::MPI::send(angularFactor,root,tag+3,mpiComm);
			Dmrg::send(su2Related,root,tag+4,mpiComm);
		}

		void recv(int root,int tag,PsimagLite::MPI::CommType mpiComm)
		{
			data.recv(root,tag,mpiComm);
			PsimagLite::MPI::recv(fermionSign,root,tag+1,mpiComm);
			PsimagLite::MPI::recv(jm,root,tag+2,mpiComm);
			PsimagLite::MPI::recv(angularFactor,root,tag+3,mpiComm);
			Dmrg::recv(su2Related,root,tag+4,mpiComm);
		}

		SparseMatrixType data;
		int fermionSign; // does this operator commute or anticommute with others of the same class on different sites
		PairType  jm; // angular momentum of this operator	
		RealType angularFactor;
		Su2RelatedType su2Related;

	private:

		void checkNotZeroMatrix(const PsimagLite::Matrix<SparseElementType>& m) const
		{
			RealType norma = norm2(m);
			RealType eps = 1e-6;
			if (norma>eps) return;

			PsimagLite::String s(__FILE__);
			s += " : " + ttos(__LINE__) + "\n";
			s += "RAW_MATRIX or COOKED_OPERATOR ";
			s += " is less than " + ttos(eps) + "\n";
			throw PsimagLite::RuntimeError(s.c_str());
		}
	};

	template<typename SparseMatrixType>
	void bcast(Operator<SparseMatrixType>& op)
	{
		PsimagLite::bcast(op.data);
		PsimagLite::MPI::bcast(op.fermionSign);
		PsimagLite::MPI::bcast(op.jm);
		PsimagLite::MPI::bcast(op.angularFactor);
		bcast(op.su2Related);
	}

	template<typename SparseMatrixType,
	         template<typename,typename> class SomeVectorTemplate,
	         typename SomeAllocator1Type,
	         typename SomeAllocator2Type>
	void fillOperator(SomeVectorTemplate<SparseMatrixType*,SomeAllocator1Type>& data,
	                  SomeVectorTemplate<Operator<SparseMatrixType>,SomeAllocator2Type>& op)
	{
		for (SizeType i=0;i<data.size();i++) {
			data[i] = &(op[i].data);
		}
	}

	template<typename SparseMatrixType>
	std::istream& operator>>(std::istream& is,Operator<SparseMatrixType>& op)
	{
		is>>op.data;
		is>>op.fermionSign;
		is>>op.jm;
		is>>op.angularFactor;
		is>>op.su2Related;
		return is;
	}

	template<typename SparseMatrixType>
	std::ostream& operator<<(std::ostream& os,const Operator<SparseMatrixType>& op)
	{
		os<<op.data;
		os<<op.fermionSign<<"\n";
		os<<op.jm.first<<" "<<op.jm.second<<"\n";
		os<<op.angularFactor<<"\n";
		os<<op.su2Related;
		return os;
	}
} // namespace Dmrg

/*@}*/
#endif

