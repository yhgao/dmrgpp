/*
Copyright (c) 2009-2017, UT-Battelle, LLC
All rights reserved

[DMRG++, Version 4.0]
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

/*! \file InitKron.h
 *
 *
 */

#ifndef INIT_KRON_HEADER_H
#define INIT_KRON_HEADER_H

#include "ProgramGlobals.h"

namespace Dmrg {

template<typename PreInitKronType>
class InitKron {

	static const bool KRON_USE_SYMMETRY = false;

public:

	typedef typename PreInitKronType::LeftRightSuperType LeftRightSuperType;
	typedef typename LeftRightSuperType::RealType RealType;
	typedef typename LeftRightSuperType::SparseMatrixType SparseMatrixType;
	typedef typename SparseMatrixType::value_type ComplexOrRealType;
	typedef typename PreInitKronType::ArrayOfMatStructType ArrayOfMatStructType;
	typedef typename ArrayOfMatStructType::VectorSizeType VectorSizeType;
	typedef typename ArrayOfMatStructType::GenIjPatchType GenIjPatchType;
	typedef typename PreInitKronType::LinkType LinkType;
	typedef typename PreInitKronType::ModelType ModelType;
	typedef typename ModelType::LinkProductStructType LinkProductStructType;

	InitKron(PreInitKronType& preInitKron, bool loadBalance)
	    : preInitKron_(preInitKron), loadBalance_(loadBalance)
	{}

	bool useSymmetry() const { return KRON_USE_SYMMETRY; }

	bool loadBalance() const
	{
		return loadBalance_;
//		return (model_.params().options.find("KronNoLoadBalance")
//		        == PsimagLite::String::npos);
	}

	const ArrayOfMatStructType& xc(SizeType ic) const
	{
		return preInitKron_.xc(ic);
	}

	const ArrayOfMatStructType& yc(SizeType ic) const
	{
		return preInitKron_.yc(ic);
	}

	const VectorSizeType& patch(typename GenIjPatchType::LeftOrRightEnumType i) const
	{
		return ijpatches_(i);
	}

//	const LeftRightSuperType& lrs() const
//	{
//		return modelHelper_.leftRightSuper();
//	}

//	SizeType offset() const
//	{
//		SizeType m = modelHelper_.m();
//		return modelHelper_.leftRightSuper().super().partition(m);
//	}

//	SizeType size() const { return modelHelper_.size(); }

	SizeType connections() const { return preInitKron_.connections(); }

	const ComplexOrRealType& value(SizeType i) const
	{
		return preInitKron_.value(i);
	}

private:

	InitKron(const InitKron& other);

	InitKron& operator=(const InitKron& other);

	PreInitKronType& preInitKron_;
	bool loadBalance_;
}; //class InitKron
} // namespace PsimagLite

/*@}*/

#endif // INIT_KRON_HEADER_H
