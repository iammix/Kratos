// KRATOS  ___|  |                   |                   |
//       \___ \  __|  __| |   |  __| __| |   |  __| _` | |
//             | |   |    |   | (    |   |   | |   (   | |
//       _____/ \__|_|   \__,_|\___|\__|\__,_|_|  \__,_|_| MECHANICS
//
//  License:		 BSD License
//					 license: structural_mechanics_application/license.txt
//
//  Main authors:    Peter Wilson
//

#include "shell_thick_element_3D3N.hpp"
#include "custom_utilities/shellt3_corotational_coordinate_transformation.hpp"
#include "structural_mechanics_application_variables.h"

#include "geometries/triangle_3d_3.h"

#include <string>
#include <iomanip>

#define OPT_NUM_NODES 3
#define OPT_STRAIN_SIZE 6
#define OPT_NUM_DOFS 18

//----------------------------------------
// preprocessors for the integration
// method used by this element.

//#define OPT_1_POINT_INTEGRATION

#ifdef OPT_1_POINT_INTEGRATION
#define OPT_INTEGRATION_METHOD Kratos::GeometryData::GI_GAUSS_1
#define OPT_NUM_GP 1
#else
#define OPT_INTEGRATION_METHOD Kratos::GeometryData::GI_GAUSS_2
#define OPT_NUM_GP 3
#endif // OPT_1_POINT_INTEGRATION

//----------------------------------------
// preprocessors to handle the output
// in case of 3 integration points

//#define OPT_USES_INTERIOR_GAUSS_POINTS

#ifdef OPT_1_POINT_INTEGRATION
#define OPT_INTERPOLATE_RESULTS_TO_STANDARD_GAUSS_POINTS(X)
#else
#ifdef OPT_USES_INTERIOR_GAUSS_POINTS
#define OPT_INTERPOLATE_RESULTS_TO_STANDARD_GAUSS_POINTS(X)
#else
#define OPT_INTERPOLATE_RESULTS_TO_STANDARD_GAUSS_POINTS(X) Utilities::InterpToStandardGaussPoints(X)
#endif // OPT_USES_INTERIOR_GAUSS_POINTS
#endif // OPT_1_POINT_INTEGRATION

namespace Kratos
{
	namespace Utilities
	{
		inline void InterpToStandardGaussPoints(double& v1, double& v2,
			double& v3)
		{
			double vg1 = v1;
			double vg2 = v2;
			double vg3 = v3;
#ifdef OPT_AVARAGE_RESULTS
			v1 = (vg1 + vg2 + vg3) / 3.0;
			v2 = (vg1 + vg2 + vg3) / 3.0;
			v3 = (vg1 + vg2 + vg3) / 3.0;
#else
			v1 = (2.0*vg1) / 3.0 - vg2 / 3.0 + (2.0*vg3) / 3.0;
			v2 = (2.0*vg1) / 3.0 + (2.0*vg2) / 3.0 - vg3 / 3.0;
			v3 = (2.0*vg2) / 3.0 - vg1 / 3.0 + (2.0*vg3) / 3.0;
#endif // OPT_AVARAGE_RESULTS
		}

		inline void InterpToStandardGaussPoints(std::vector< double >& v)
		{
			if (v.size() != 3) return;
			InterpToStandardGaussPoints(v[0], v[1], v[2]);
		}

		inline void InterpToStandardGaussPoints(std::vector< array_1d<double,
			3> >& v)
		{
			if (v.size() != 3) return;
			for (size_t i = 0; i < 3; i++)
				InterpToStandardGaussPoints(v[0][i], v[1][i], v[2][i]);
		}

		inline void InterpToStandardGaussPoints(std::vector< array_1d<double,
			6> >& v)
		{
			if (v.size() != 3) return;
			for (size_t i = 0; i < 6; i++)
				InterpToStandardGaussPoints(v[0][i], v[1][i], v[2][i]);
		}

		inline void InterpToStandardGaussPoints(std::vector< Vector >& v)
		{
			if (v.size() != 3) return;
			size_t ncomp = v[0].size();
			for (int i = 1; i < 3; i++)
				if (v[i].size() != ncomp)
					return;
			for (size_t i = 0; i < ncomp; i++)
				InterpToStandardGaussPoints(v[0][i], v[1][i], v[2][i]);
		}

		inline void InterpToStandardGaussPoints(std::vector< Matrix >& v)
		{
			if (v.size() != 3) return;
			size_t nrows = v[0].size1();
			size_t ncols = v[0].size2();
			for (int i = 1; i < 3; i++)
				if (v[i].size1() != nrows || v[i].size2() != ncols)
					return;
			for (size_t i = 0; i < nrows; i++)
				for (size_t j = 0; j < ncols; j++)
					InterpToStandardGaussPoints
					(v[0](i, j), v[1](i, j), v[2](i, j));
		}
	}

	// =========================================================================
	//
	// CalculationData
	//
	// =========================================================================

	ShellThickElement3D3N::CalculationData::CalculationData
	(const CoordinateTransformationBasePointerType& pCoordinateTransformation,
		const ProcessInfo& rCurrentProcessInfo)
		: LCS0(pCoordinateTransformation->CreateReferenceCoordinateSystem())
		, LCS(pCoordinateTransformation->CreateLocalCoordinateSystem())
		, CurrentProcessInfo(rCurrentProcessInfo)

	{
	}

	// =========================================================================
	//
	// Class ShellThickElement3D3N
	//
	// =========================================================================

	ShellThickElement3D3N::ShellThickElement3D3N(IndexType NewId,
		GeometryType::Pointer pGeometry,
		bool NLGeom)
		: Element(NewId, pGeometry)
		, mpCoordinateTransformation(NLGeom ?
			new ShellT3_CorotationalCoordinateTransformation(pGeometry) :
			new ShellT3_CoordinateTransformation(pGeometry))
	{
		mThisIntegrationMethod = OPT_INTEGRATION_METHOD;
	}

	ShellThickElement3D3N::ShellThickElement3D3N(IndexType NewId,
		GeometryType::Pointer pGeometry,
		PropertiesType::Pointer pProperties,
		bool NLGeom)
		: Element(NewId, pGeometry, pProperties)
		, mpCoordinateTransformation(NLGeom ?
			new ShellT3_CorotationalCoordinateTransformation(pGeometry) :
			new ShellT3_CoordinateTransformation(pGeometry))
	{
		mThisIntegrationMethod = OPT_INTEGRATION_METHOD;
	}

	ShellThickElement3D3N::ShellThickElement3D3N(IndexType NewId,
		GeometryType::Pointer pGeometry,
		PropertiesType::Pointer pProperties,
		CoordinateTransformationBasePointerType pCoordinateTransformation)
		: Element(NewId, pGeometry, pProperties)
		, mpCoordinateTransformation(pCoordinateTransformation)
	{
		mThisIntegrationMethod = OPT_INTEGRATION_METHOD;
	}

	ShellThickElement3D3N::~ShellThickElement3D3N()
	{
	}

	Element::Pointer ShellThickElement3D3N::Create(IndexType NewId,
		NodesArrayType const& ThisNodes,
		PropertiesType::Pointer pProperties) const
	{
		GeometryType::Pointer newGeom(GetGeometry().Create(ThisNodes));
		return boost::make_shared< ShellThickElement3D3N >(NewId, newGeom,
			pProperties, mpCoordinateTransformation->Create(newGeom));
	}

	ShellThickElement3D3N::IntegrationMethod
		ShellThickElement3D3N::GetIntegrationMethod() const
	{
		return mThisIntegrationMethod;
	}

	void ShellThickElement3D3N::Initialize()
	{
		KRATOS_TRY

			const GeometryType & geom = GetGeometry();
		PropertiesType & props = GetProperties();

		if (geom.PointsNumber() != OPT_NUM_NODES)
			KRATOS_THROW_ERROR(std::logic_error,
				"ShellThickElement3D3N Element - Wrong number of nodes",
				geom.PointsNumber());

		const GeometryType::IntegrationPointsArrayType & integrationPoints =
			geom.IntegrationPoints(GetIntegrationMethod());

		if (integrationPoints.size() != OPT_NUM_GP)
			KRATOS_THROW_ERROR(std::logic_error,
				"ShellThickElement3D3N Element - Wrong integration scheme",
				integrationPoints.size());

		if (mSections.size() != OPT_NUM_GP)
		{
			const Matrix & shapeFunctionsValues =
				geom.ShapeFunctionsValues(GetIntegrationMethod());

			ShellCrossSection::Pointer theSection;
			if (props.Has(SHELL_CROSS_SECTION))
			{
				theSection = props[SHELL_CROSS_SECTION];
			}
			else if (theSection->CheckIsOrthotropic(props))
			{
				// make new instance of shell cross section
				theSection = ShellCrossSection::Pointer(new ShellCrossSection());

				// Assign orthotropic material law for entire element
				LinearElasticOrthotropic2DLaw OrthoLaw;
				props.SetValue(CONSTITUTIVE_LAW, OrthoLaw.Clone());

				// Parse material properties for each layer
				Element* thisElement = this;
				theSection->ParseOrthotropicPropertyMatrix(props, thisElement);
			}
			else
			{
				theSection = ShellCrossSection::Pointer(new ShellCrossSection());
				theSection->BeginStack();
				theSection->
					AddPly(props[THICKNESS], 0.0, 5, this->pGetProperties());
				theSection->EndStack();
			}

			mSections.clear();
			for (int i = 0; i < OPT_NUM_GP; i++)
			{
				ShellCrossSection::Pointer sectionClone = theSection->Clone();
				sectionClone->SetSectionBehavior(ShellCrossSection::Thick);
				sectionClone->InitializeCrossSection(props, geom,
					row(shapeFunctionsValues, i));
				mSections.push_back(sectionClone);
			}
		}

		mpCoordinateTransformation->Initialize();

		this->SetupOrientationAngles();

		KRATOS_CATCH("")
	}

	void ShellThickElement3D3N::ResetConstitutiveLaw()
	{
		KRATOS_TRY

			const GeometryType & geom = GetGeometry();
		const Matrix & shapeFunctionsValues =
			geom.ShapeFunctionsValues(GetIntegrationMethod());

		const Properties& props = GetProperties();
		for (SizeType i = 0; i < mSections.size(); i++)
			mSections[i]->ResetCrossSection(props, geom,
				row(shapeFunctionsValues, i));

		KRATOS_CATCH("")
	}

	void ShellThickElement3D3N::EquationIdVector(EquationIdVectorType& rResult,
		ProcessInfo& rCurrentProcessInfo)
	{
		if (rResult.size() != OPT_NUM_DOFS)
			rResult.resize(OPT_NUM_DOFS, false);

		GeometryType & geom = this->GetGeometry();

		for (SizeType i = 0; i < geom.size(); i++)
		{
			int index = i * 6;
			NodeType & iNode = geom[i];

			rResult[index] = iNode.GetDof(DISPLACEMENT_X).EquationId();
			rResult[index + 1] = iNode.GetDof(DISPLACEMENT_Y).EquationId();
			rResult[index + 2] = iNode.GetDof(DISPLACEMENT_Z).EquationId();

			rResult[index + 3] = iNode.GetDof(ROTATION_X).EquationId();
			rResult[index + 4] = iNode.GetDof(ROTATION_Y).EquationId();
			rResult[index + 5] = iNode.GetDof(ROTATION_Z).EquationId();
		}
	}

	void ShellThickElement3D3N::GetDofList(DofsVectorType& ElementalDofList,
		ProcessInfo& CurrentProcessInfo)
	{
		ElementalDofList.resize(0);
		ElementalDofList.reserve(OPT_NUM_DOFS);

		GeometryType & geom = this->GetGeometry();

		for (SizeType i = 0; i < geom.size(); i++)
		{
			NodeType & iNode = geom[i];

			ElementalDofList.push_back(iNode.pGetDof(DISPLACEMENT_X));
			ElementalDofList.push_back(iNode.pGetDof(DISPLACEMENT_Y));
			ElementalDofList.push_back(iNode.pGetDof(DISPLACEMENT_Z));

			ElementalDofList.push_back(iNode.pGetDof(ROTATION_X));
			ElementalDofList.push_back(iNode.pGetDof(ROTATION_Y));
			ElementalDofList.push_back(iNode.pGetDof(ROTATION_Z));
		}
	}

	int ShellThickElement3D3N::Check(const ProcessInfo& rCurrentProcessInfo)
	{
		KRATOS_TRY

			GeometryType& geom = GetGeometry();

		// verify that the variables are correctly initialized
		if (DISPLACEMENT.Key() == 0)
			KRATOS_THROW_ERROR(std::invalid_argument,
				"DISPLACEMENT has Key zero! (check if the application is correctly registered", "");

		if (ROTATION.Key() == 0)
			KRATOS_THROW_ERROR(std::invalid_argument,
				"ROTATION has Key zero! (check if the application is correctly registered", "");

		if (VELOCITY.Key() == 0)
			KRATOS_THROW_ERROR(std::invalid_argument,
				"VELOCITY has Key zero! (check if the application is correctly registered", "");

		if (ACCELERATION.Key() == 0)
			KRATOS_THROW_ERROR(std::invalid_argument,
				"ACCELERATION has Key zero! (check if the application is correctly registered", "");

		if (DENSITY.Key() == 0)
			KRATOS_THROW_ERROR(std::invalid_argument,
				"DENSITY has Key zero! (check if the application is correctly registered", "");

		if (SHELL_CROSS_SECTION.Key() == 0)
			KRATOS_THROW_ERROR(std::invalid_argument,
				"SHELL_CROSS_SECTION has Key zero! (check if the application is correctly registered", "");

		if (THICKNESS.Key() == 0)
			KRATOS_THROW_ERROR(std::invalid_argument,
				"THICKNESS has Key zero! (check if the application is correctly registered", "");

		if (CONSTITUTIVE_LAW.Key() == 0)
			KRATOS_THROW_ERROR(std::invalid_argument,
				"CONSTITUTIVE_LAW has Key zero! (check if the application is correctly registered", "");

		// verify that the dofs exist
		for (unsigned int i = 0; i < geom.size(); i++)
		{
			if (geom[i].SolutionStepsDataHas(DISPLACEMENT) == false)
				KRATOS_THROW_ERROR(std::invalid_argument,
					"missing variable DISPLACEMENT on node ", geom[i].Id());

			if (geom[i].HasDofFor(DISPLACEMENT_X) == false ||
				geom[i].HasDofFor(DISPLACEMENT_Y) == false ||
				geom[i].HasDofFor(DISPLACEMENT_Z) == false)
				KRATOS_THROW_ERROR(std::invalid_argument,
					"missing one of the dofs for the variable DISPLACEMENT on node ",
					GetGeometry()[i].Id());

			if (geom[i].SolutionStepsDataHas(ROTATION) == false)
				KRATOS_THROW_ERROR(std::invalid_argument,
					"missing variable ROTATION on node ", geom[i].Id());

			if (geom[i].HasDofFor(ROTATION_X) == false ||
				geom[i].HasDofFor(ROTATION_Y) == false ||
				geom[i].HasDofFor(ROTATION_Z) == false)
				KRATOS_THROW_ERROR(std::invalid_argument,
					"missing one of the dofs for the variable ROTATION on node ",
					geom[i].Id());

			if (geom[i].GetBufferSize() < 2)
				KRATOS_THROW_ERROR(std::logic_error,
					"This Element needs at least a buffer size = 2", "");
		}

		// check properties
		if (this->pGetProperties() == NULL)
			KRATOS_THROW_ERROR(std::logic_error,
				"Properties not provided for element ", this->Id());

		const PropertiesType & props = this->GetProperties();

		if (props.Has(SHELL_CROSS_SECTION))
		{
			// if the user specified a cross section ...

			const ShellCrossSection::Pointer & section =
				props[SHELL_CROSS_SECTION];
			if (section == NULL)
				KRATOS_THROW_ERROR(std::logic_error,
					"SHELL_CROSS_SECTION not provided for element ", this->Id());

			section->Check(props, geom, rCurrentProcessInfo);
		}
		else if (props.Has(SHELL_ORTHOTROPIC_LAYERS))
		{
			// perform orthotropic check later in shell_cross_section
		}
		else
		{
			// ... allow the automatic creation of a homogeneous section from a
			// material and a thickness

			if (!props.Has(CONSTITUTIVE_LAW))
				KRATOS_THROW_ERROR(std::logic_error,
					"CONSTITUTIVE_LAW not provided for element ", this->Id());

			const ConstitutiveLaw::Pointer& claw = props[CONSTITUTIVE_LAW];
			if (claw == NULL)
				KRATOS_THROW_ERROR(std::logic_error,
					"CONSTITUTIVE_LAW not provided for element ", this->Id());

			if (!props.Has(THICKNESS))
				KRATOS_THROW_ERROR(std::logic_error,
					"THICKNESS not provided for element ", this->Id());

			if (props[THICKNESS] <= 0.0)
				KRATOS_THROW_ERROR(std::logic_error,
					"wrong THICKNESS value provided for element ", this->Id());

			ShellCrossSection::Pointer dummySection =
				ShellCrossSection::Pointer(new ShellCrossSection());
			dummySection->BeginStack();
			dummySection->AddPly(props[THICKNESS], 0.0, 5,
				this->pGetProperties());
			dummySection->EndStack();
			dummySection->SetSectionBehavior(ShellCrossSection::Thick);
			dummySection->Check(props, geom, rCurrentProcessInfo);
		}

		return 0;

		KRATOS_CATCH("")
	}

	void ShellThickElement3D3N::CleanMemory()
	{
	}

	void ShellThickElement3D3N::GetValuesVector(Vector& values, int Step)
	{
		if (values.size() != OPT_NUM_DOFS)
			values.resize(OPT_NUM_DOFS, false);

		const GeometryType & geom = GetGeometry();

		for (SizeType i = 0; i < geom.size(); i++)
		{
			const NodeType & iNode = geom[i];
			const array_1d<double, 3>& disp =
				iNode.FastGetSolutionStepValue(DISPLACEMENT, Step);
			const array_1d<double, 3>& rot =
				iNode.FastGetSolutionStepValue(ROTATION, Step);

			int index = i * 6;
			values[index] = disp[0];
			values[index + 1] = disp[1];
			values[index + 2] = disp[2];

			values[index + 3] = rot[0];
			values[index + 4] = rot[1];
			values[index + 5] = rot[2];
		}
	}

	void ShellThickElement3D3N::GetFirstDerivativesVector(Vector& values,
		int Step)
	{
		if (values.size() != OPT_NUM_DOFS)
			values.resize(OPT_NUM_DOFS, false);

		const GeometryType & geom = GetGeometry();

		for (SizeType i = 0; i < geom.size(); i++)
		{
			const NodeType & iNode = geom[i];
			const array_1d<double, 3>& vel =
				iNode.FastGetSolutionStepValue(VELOCITY, Step);

			int index = i * 6;
			values[index] = vel[0];
			values[index + 1] = vel[1];
			values[index + 2] = vel[2];
			values[index + 3] = 0.0;
			values[index + 4] = 0.0;
			values[index + 5] = 0.0;
		}
	}

	void ShellThickElement3D3N::GetSecondDerivativesVector(Vector& values,
		int Step)
	{
		if (values.size() != OPT_NUM_DOFS)
			values.resize(OPT_NUM_DOFS, false);

		const GeometryType & geom = GetGeometry();

		for (SizeType i = 0; i < geom.size(); i++)
		{
			const NodeType & iNode = geom[i];
			const array_1d<double, 3>& acc =
				iNode.FastGetSolutionStepValue(ACCELERATION, Step);

			int index = i * 6;
			values[index] = acc[0];
			values[index + 1] = acc[1];
			values[index + 2] = acc[2];
			values[index + 3] = 0.0;
			values[index + 4] = 0.0;
			values[index + 5] = 0.0;
		}
	}

	void ShellThickElement3D3N::InitializeNonLinearIteration
	(ProcessInfo& CurrentProcessInfo)
	{
		mpCoordinateTransformation->
			InitializeNonLinearIteration(CurrentProcessInfo);

		const GeometryType & geom = this->GetGeometry();
		const Matrix & shapeFunctionsValues =
			geom.ShapeFunctionsValues(GetIntegrationMethod());
		for (SizeType i = 0; i < mSections.size(); i++)
			mSections[i]->InitializeNonLinearIteration(GetProperties(), geom,
				row(shapeFunctionsValues, i), CurrentProcessInfo);
	}

	void ShellThickElement3D3N::FinalizeNonLinearIteration(ProcessInfo& CurrentProcessInfo)
	{
		mpCoordinateTransformation->FinalizeNonLinearIteration(CurrentProcessInfo);

		const GeometryType & geom = this->GetGeometry();
		const Matrix & shapeFunctionsValues = geom.ShapeFunctionsValues(GetIntegrationMethod());
		for (SizeType i = 0; i < mSections.size(); i++)
			mSections[i]->FinalizeNonLinearIteration(GetProperties(), geom, row(shapeFunctionsValues, i), CurrentProcessInfo);
	}

	void ShellThickElement3D3N::InitializeSolutionStep(ProcessInfo& CurrentProcessInfo)
	{
		const PropertiesType& props = GetProperties();
		const GeometryType & geom = GetGeometry();
		const Matrix & shapeFunctionsValues = geom.ShapeFunctionsValues(GetIntegrationMethod());

		for (SizeType i = 0; i < mSections.size(); i++)
			mSections[i]->InitializeSolutionStep(props, geom, row(shapeFunctionsValues, i), CurrentProcessInfo);

		mpCoordinateTransformation->InitializeSolutionStep(CurrentProcessInfo);
	}

	void ShellThickElement3D3N::FinalizeSolutionStep(ProcessInfo& CurrentProcessInfo)
	{
		const PropertiesType& props = GetProperties();
		const GeometryType& geom = GetGeometry();
		const Matrix & shapeFunctionsValues = geom.ShapeFunctionsValues(GetIntegrationMethod());

		for (SizeType i = 0; i < mSections.size(); i++)
			mSections[i]->FinalizeSolutionStep(props, geom, row(shapeFunctionsValues, i), CurrentProcessInfo);

		mpCoordinateTransformation->FinalizeSolutionStep(CurrentProcessInfo);
	}

	void ShellThickElement3D3N::CalculateMassMatrix(MatrixType& rMassMatrix, ProcessInfo& rCurrentProcessInfo)
	{
		if ((rMassMatrix.size1() != OPT_NUM_DOFS) || (rMassMatrix.size2() != OPT_NUM_DOFS))
			rMassMatrix.resize(OPT_NUM_DOFS, OPT_NUM_DOFS, false);
		noalias(rMassMatrix) = ZeroMatrix(OPT_NUM_DOFS, OPT_NUM_DOFS);

		// Compute the local coordinate system.
		ShellT3_LocalCoordinateSystem referenceCoordinateSystem(
			mpCoordinateTransformation->CreateReferenceCoordinateSystem());

		// Average mass per unit area over the whole element
		double av_mass_per_unit_area = 0.0;
		for (size_t i = 0; i < OPT_NUM_GP; i++)
			av_mass_per_unit_area += mSections[i]->CalculateMassPerUnitArea();
		av_mass_per_unit_area /= double(OPT_NUM_GP);

		// Flag for consistent or lumped mass matrix
		bool bconsistent_matrix = false;

		// Consistent mass matrix
		if (bconsistent_matrix)
		{
			// General matrix form as per Felippa plane stress CST eqn 31.27:
			// http://kis.tu.kielce.pl/mo/COLORADO_FEM/colorado/IFEM.Ch31.pdf
			// 
			// Density and thickness are averaged over element.

			// Average thickness over the whole element
			double thickness = 0.0;
			for (size_t i = 0; i < OPT_NUM_GP; i++)
				thickness += mSections[i]->GetThickness();
			thickness /= double(OPT_NUM_GP);

			// Populate mass matrix with integation results
			for (size_t row = 0; row < 18; row++)
			{
				if (row % 6 < 3)
				{
					// translational entry
					for (size_t col = 0; col < 3; col++)
					{
						rMassMatrix(row, 6 * col + row % 6) = 1.0;
					}
				}
				else
				{
					// rotational entry
					for (size_t col = 0; col < 3; col++)
					{
						rMassMatrix(row, 6 * col + row % 6) = 
							thickness*thickness / 12.0;
					}
				}

				// Diagonal entry
				rMassMatrix(row, row) *= 2.0;
			}

			rMassMatrix *= 
				av_mass_per_unit_area*referenceCoordinateSystem.Area() / 12.0;
		}// Consistent mass matrix
		else
		{
			// Lumped mass matrix

 			// lumped area
			double lump_area = referenceCoordinateSystem.Area() / 3.0;

			// loop on nodes
			for (size_t i = 0; i < 3; i++)
			{
				size_t index = i * 6;

				double nodal_mass = av_mass_per_unit_area * lump_area;

				// translational mass
				rMassMatrix(index, index) = nodal_mass;
				rMassMatrix(index + 1, index + 1) = nodal_mass;
				rMassMatrix(index + 2, index + 2) = nodal_mass;

				// rotational mass - neglected for the moment...
			}
		}// Lumped mass matrix
	}

	void ShellThickElement3D3N::CalculateDampingMatrix(MatrixType& rDampingMatrix, ProcessInfo& rCurrentProcessInfo)
	{
		if ((rDampingMatrix.size1() != OPT_NUM_DOFS) || (rDampingMatrix.size2() != OPT_NUM_DOFS))
			rDampingMatrix.resize(OPT_NUM_DOFS, OPT_NUM_DOFS, false);

		noalias(rDampingMatrix) = ZeroMatrix(OPT_NUM_DOFS, OPT_NUM_DOFS);
	}

	void ShellThickElement3D3N::CalculateLocalSystem(MatrixType& rLeftHandSideMatrix,
		VectorType& rRightHandSideVector,
		ProcessInfo& rCurrentProcessInfo)
	{
		CalculateAll(rLeftHandSideMatrix, rRightHandSideVector, rCurrentProcessInfo, true, true);
	}

	void ShellThickElement3D3N::CalculateRightHandSide(VectorType& rRightHandSideVector,
		ProcessInfo& rCurrentProcessInfo)
	{
		Matrix dummy;
		CalculateAll(dummy, rRightHandSideVector, rCurrentProcessInfo, true, true);
	}

	// =====================================================================================
	//
	// Class ShellThickElement3D3N - Results on Gauss Points
	//
	// =====================================================================================

	void ShellThickElement3D3N::GetValueOnIntegrationPoints(const Variable<double>& rVariable,
		std::vector<double>& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
		// resize output
		if (rValues.size() != OPT_NUM_GP)
			rValues.resize(OPT_NUM_GP);

		if (rVariable == VON_MISES_STRESS ||
			rVariable == VON_MISES_STRESS_TOP_SURFACE ||
			rVariable == VON_MISES_STRESS_MIDDLE_SURFACE ||
			rVariable == VON_MISES_STRESS_BOTTOM_SURFACE)
		{
			// Initialize common calculation variables
			CalculationData data(mpCoordinateTransformation, rCurrentProcessInfo);
			data.CalculateLHS = true;
			data.CalculateRHS = true;
			InitializeCalculationData(data);
			double von_mises_top, von_mises_mid, von_mises_bottom;

			// Get the current displacements in global coordinate system and 
			// transform to reference local system
			ShellT3_LocalCoordinateSystem referenceCoordinateSystem(
				mpCoordinateTransformation->CreateReferenceCoordinateSystem());
			MatrixType Rdisp(18, 18);
			referenceCoordinateSystem.ComputeTotalRotationMatrix(Rdisp);
			data.localDisplacements = prod(Rdisp, data.globalDisplacements);
			
			// Get strains
			noalias(data.generalizedStrains) = prod(data.B, data.localDisplacements);

			// Calculate the response of the Cross Section - setup for 1 GP
			data.gpIndex = 0;
			ShellCrossSection::Pointer & section = mSections[0];
			CalculateSectionResponse(data);
			data.generalizedStresses = prod(data.D, data.generalizedStrains);
			// (perform the calculation manually (outside material law) to
			// ensure stabilization is used on the transverse shear part
			// of the material matrix

			// recover stresses
			CalculateStressesFromForceResultants(data.generalizedStresses,
				section->GetThickness());

			// account for orientation
			if (section->GetOrientationAngle() != 0.0)
			{
				Matrix R(8, 8);
				section->GetRotationMatrixForGeneralizedStresses
				(-(section->GetOrientationAngle()), R);
				data.generalizedStresses = prod(R, data.generalizedStresses);
			}

			// calc von mises stresses at top, mid and bottom surfaces for
			// thick shell
			double sxx, syy, sxy, sxz, syz;

			// top surface: membrane and +bending contributions
			//				(no transverse shear)
			sxx = data.generalizedStresses[0] + data.generalizedStresses[3];
			syy = data.generalizedStresses[1] + data.generalizedStresses[4];
			sxy = data.generalizedStresses[2] + data.generalizedStresses[5];
			von_mises_top = sxx*sxx - sxx*syy + syy*syy + 3.0*sxy*sxy;

			// mid surface: membrane and transverse shear contributions
			//				(no bending)
			sxx = data.generalizedStresses[0];
			syy = data.generalizedStresses[1];
			sxy = data.generalizedStresses[2];
			sxz = data.generalizedStresses[6];
			syz = data.generalizedStresses[7];
			von_mises_mid = sxx*sxx - sxx*syy + syy*syy +
				3.0*(sxy*sxy + sxz*sxz + syz*syz);

			// bottom surface:	membrane and -bending contributions
			//					(no transverse shear)
			sxx = data.generalizedStresses[0] - data.generalizedStresses[3];
			syy = data.generalizedStresses[1] - data.generalizedStresses[4];
			sxy = data.generalizedStresses[2] - data.generalizedStresses[5];
			von_mises_bottom = sxx*sxx - sxx*syy + syy*syy + 3.0*sxy*sxy;


			// loop over gauss points - for output only
			for (unsigned int gauss_point = 0; gauss_point < OPT_NUM_GP; ++gauss_point)
			{
				// Output requested quantity
				if (rVariable == VON_MISES_STRESS_TOP_SURFACE)
				{
					rValues[gauss_point] = sqrt(von_mises_top);
				}
				else if (rVariable == VON_MISES_STRESS_MIDDLE_SURFACE)
				{
					rValues[gauss_point] = sqrt(von_mises_mid);
				}
				else if (rVariable == VON_MISES_STRESS_BOTTOM_SURFACE)
				{
					rValues[gauss_point] = sqrt(von_mises_bottom);
				}
				else if (rVariable == VON_MISES_STRESS)
				{
					// take the greatest value and output
					rValues[gauss_point] =
						sqrt(std::max(von_mises_top,
							std::max(von_mises_mid, von_mises_bottom)));
				}
			}
		}
		else if (rVariable == TSAI_WU_RESERVE_FACTOR)
		{
			// resize output
			if (rValues.size() != OPT_NUM_GP)
				rValues.resize(OPT_NUM_GP);

			// Initialize common calculation variables
			CalculationData data(mpCoordinateTransformation, rCurrentProcessInfo);
			data.CalculateLHS = true;
			data.CalculateRHS = true;
			InitializeCalculationData(data);
			data.gpIndex = 0;

			// Get the current displacements in global coordinate system and 
			// transform to reference local system
			ShellT3_LocalCoordinateSystem referenceCoordinateSystem(
				mpCoordinateTransformation->CreateReferenceCoordinateSystem());
			MatrixType Rdisp(18, 18);
			referenceCoordinateSystem.ComputeTotalRotationMatrix(Rdisp);
			data.localDisplacements = prod(Rdisp, data.globalDisplacements);

			// Get strains
			noalias(data.generalizedStrains) = prod(data.B, data.localDisplacements);

			// Get all laminae strengths
			PropertiesType & props = GetProperties();
			ShellCrossSection::Pointer & section = mSections[0];
			std::vector<Matrix> Laminae_Strengths =
				std::vector<Matrix>(section->NumberOfPlies());
			for (unsigned int ply = 0; ply < section->NumberOfPlies(); ply++)
			{
				Laminae_Strengths[ply].resize(3, 3, 0.0);
				Laminae_Strengths[ply].clear();
			}
			section->GetLaminaeStrengths(Laminae_Strengths, props);

			// Define variables
			Matrix R(8, 8);

			// Retrieve ply orientations
			Vector ply_orientation(section->NumberOfPlies());
			section->GetLaminaeOrientation(ply_orientation);
			double total_rotation = 0.0;

			//Calculate lamina stresses
			CalculateLaminaStrains(data);
			CalculateLaminaStresses(data);

			// Rotate lamina stress to lamina material principal directions
			for (unsigned int ply = 0; ply < section->NumberOfPlies(); ply++)
			{
				total_rotation = -ply_orientation[ply] - (section->GetOrientationAngle());
				section->GetRotationMatrixForGeneralizedStresses(total_rotation, R);
				//top surface of current ply
				data.rlaminateStresses[2 * ply] = prod(R, data.rlaminateStresses[2 * ply]);
				//bottom surface of current ply
				data.rlaminateStresses[2 * ply + 1] = prod(R, data.rlaminateStresses[2 * ply + 1]);
			}

			// Calculate Tsai-Wu criterion for each ply, take min of all plies
			double min_tsai_wu = 0.0;
			double temp_tsai_wu = 0.0;
			for (unsigned int ply = 0; ply < section->NumberOfPlies(); ply++)
			{
				temp_tsai_wu = CalculateTsaiWuPlaneStress(data, Laminae_Strengths[ply], ply);
				if (ply == 0)
				{
					min_tsai_wu = temp_tsai_wu;
				}
				else if (temp_tsai_wu < min_tsai_wu)
				{
					min_tsai_wu = temp_tsai_wu;
				}
			}		

			// Gauss Loop
			for (unsigned int gauss_point = 0; gauss_point < OPT_NUM_GP; ++gauss_point)
			{
				// Output min Tsai-Wu result
				rValues[gauss_point] = min_tsai_wu;

			}// Gauss loop

		} // Tsai wu 
		else
		{
			for (int i = 0; i < OPT_NUM_GP; i++)
				mSections[i]->GetValue(rVariable, rValues[i]);
		}

		OPT_INTERPOLATE_RESULTS_TO_STANDARD_GAUSS_POINTS(rValues);
	}

	void ShellThickElement3D3N::GetValueOnIntegrationPoints(const Variable<Vector>& rVariable,
		std::vector<Vector>& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
	}

	void ShellThickElement3D3N::GetValueOnIntegrationPoints(const Variable<Matrix>& rVariable,
		std::vector<Matrix>& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
		if (TryGetValueOnIntegrationPoints_GeneralizedStrainsOrStresses(rVariable, rValues, rCurrentProcessInfo)) return;
	}

	void ShellThickElement3D3N::GetValueOnIntegrationPoints(const Variable<array_1d<double, 3> >& rVariable,
		std::vector<array_1d<double, 3> >& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
		if (TryGetValueOnIntegrationPoints_MaterialOrientation(rVariable, rValues, rCurrentProcessInfo)) return;
	}

	void ShellThickElement3D3N::GetValueOnIntegrationPoints(const Variable<array_1d<double, 6> >& rVariable,
		std::vector<array_1d<double, 6> >& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
	}

	void ShellThickElement3D3N::CalculateOnIntegrationPoints(const Variable<double>& rVariable, std::vector<double>& rValues, const ProcessInfo & rCurrentProcessInfo)
	{
		GetValueOnIntegrationPoints(rVariable, rValues, rCurrentProcessInfo);
	}

	void ShellThickElement3D3N::CalculateOnIntegrationPoints(const Variable<Vector>& rVariable,
		std::vector<Vector>& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
		GetValueOnIntegrationPoints(rVariable, rValues, rCurrentProcessInfo);
	}

	void ShellThickElement3D3N::CalculateOnIntegrationPoints(const Variable<Matrix>& rVariable,
		std::vector<Matrix>& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
		GetValueOnIntegrationPoints(rVariable, rValues, rCurrentProcessInfo);
	}

	void ShellThickElement3D3N::CalculateOnIntegrationPoints(const Variable<array_1d<double, 3> >& rVariable,
		std::vector<array_1d<double, 3> >& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
		GetValueOnIntegrationPoints(rVariable, rValues, rCurrentProcessInfo);
	}

	void ShellThickElement3D3N::CalculateOnIntegrationPoints(const Variable<array_1d<double, 6> >& rVariable,
		std::vector<array_1d<double, 6> >& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
		GetValueOnIntegrationPoints(rVariable, rValues, rCurrentProcessInfo);
	}

	void ShellThickElement3D3N::SetCrossSectionsOnIntegrationPoints(std::vector< ShellCrossSection::Pointer >& crossSections)
	{
		KRATOS_TRY
			if (crossSections.size() != OPT_NUM_GP)
				KRATOS_THROW_ERROR(std::logic_error, "The number of cross section is wrong", crossSections.size());
		mSections.clear();
		for (SizeType i = 0; i < crossSections.size(); i++)
			mSections.push_back(crossSections[i]);
		this->SetupOrientationAngles();
		KRATOS_CATCH("")
	}

	// =====================================================================================
	//
	// Class ShellThickElement3D3N - Private methods
	//
	// =====================================================================================

	void ShellThickElement3D3N::CalculateStressesFromForceResultants
	(VectorType& rstresses, const double& rthickness)
	{
		// Refer http://www.colorado.edu/engineering/CAS/courses.d/AFEM.d/AFEM.Ch20.d/AFEM.Ch20.pdf

		// membrane forces -> in-plane stresses (av. across whole thickness)
		rstresses[0] /= rthickness;
		rstresses[1] /= rthickness;
		rstresses[2] /= rthickness;

		// bending moments -> peak in-plane stresses (@ top and bottom surface)
		rstresses[3] *= 6.0 / (rthickness*rthickness);
		rstresses[4] *= 6.0 / (rthickness*rthickness);
		rstresses[5] *= 6.0 / (rthickness*rthickness);

		// shear forces -> peak shear stresses (@ midsurface)
		rstresses[6] *= 1.5 / rthickness;
		rstresses[7] *= 1.5 / rthickness;
	}

	void ShellThickElement3D3N::CalculateLaminaStrains(CalculationData& data)
	{
		ShellCrossSection::Pointer& section = mSections[data.gpIndex];

		// Get laminate properties
		double thickness = section->GetThickness();
		double z_current = thickness / -2.0; // start from the top of the 1st layer

		// Establish current strains at the midplane
		// (element coordinate system)
		double e_x = data.generalizedStrains[0];
		double e_y = data.generalizedStrains[1];
		double e_xy = data.generalizedStrains[2];	//this is still engineering
													//strain (2xtensorial shear)
		double kap_x = data.generalizedStrains[3];
		double kap_y = data.generalizedStrains[4];
		double kap_xy = data.generalizedStrains[5];	//this is still engineering
													//strain (2xtensorial shear)

		// Get ply thicknesses
		Vector ply_thicknesses = Vector(section->NumberOfPlies(), 0.0);
		section->GetPlyThicknesses(ply_thicknesses);

		// Resize output vector. 2 Surfaces for each ply
		data.rlaminateStrains.resize(2 * section->NumberOfPlies());
		for (unsigned int i = 0; i < 2 * section->NumberOfPlies(); i++)
		{
			data.rlaminateStrains[i].resize(8, false);
			data.rlaminateStrains[i].clear();
		}

		// Loop over all plies - start from top ply, top surface
		for (unsigned int plyNumber = 0;
			plyNumber < section->NumberOfPlies(); ++plyNumber)
		{
			// Calculate strains at top surface, arranged in columns.
			// (element coordinate system)
			data.rlaminateStrains[2 * plyNumber][0] = e_x + z_current*kap_x;
			data.rlaminateStrains[2 * plyNumber][1] = e_y + z_current*kap_y;
			data.rlaminateStrains[2 * plyNumber][2] = e_xy + z_current*kap_xy;

			if (data.parabolic_composite_transverse_shear_strains)
			{
				// assume parabolic transverse shear strain dist across whole
				// laminate
				data.rlaminateStrains[2 * plyNumber][6] =
					1.5*(1.0 - 4.0 * z_current*z_current / thickness / thickness)*
					data.generalizedStrains[6];
				data.rlaminateStrains[2 * plyNumber][7] =
					1.5*(1.0 - 4.0 * z_current*z_current / thickness / thickness)*
					data.generalizedStrains[7];
			}
			else
			{
				// constant transverse shear strain dist
				data.rlaminateStrains[2 * plyNumber][6] = data.generalizedStrains[6];
				data.rlaminateStrains[2 * plyNumber][7] = data.generalizedStrains[7];
			}
			

			// Move to bottom surface of current layer
			z_current += ply_thicknesses[plyNumber];

			// Calculate strains at bottom surface, arranged in columns
			// (element coordinate system)
			data.rlaminateStrains[2 * plyNumber + 1][0] = e_x + z_current*kap_x;
			data.rlaminateStrains[2 * plyNumber + 1][1] = e_y + z_current*kap_y;
			data.rlaminateStrains[2 * plyNumber + 1][2] = e_xy + z_current*kap_xy;

			if (data.parabolic_composite_transverse_shear_strains)
			{
				data.rlaminateStrains[2 * plyNumber + 1][6] =
					1.5*(1.0 - 4.0 * z_current*z_current / thickness / thickness)*
					data.generalizedStrains[6];
				data.rlaminateStrains[2 * plyNumber + 1][7] =
					1.5*(1.0 - 4.0 * z_current*z_current / thickness / thickness)*
					data.generalizedStrains[7];
			}
			else
			{
				data.rlaminateStrains[2 * plyNumber+1][6] = data.generalizedStrains[6];
				data.rlaminateStrains[2 * plyNumber+1][7] = data.generalizedStrains[7];
			}
		}
	}

	void ShellThickElement3D3N::CalculateLaminaStresses(CalculationData& data)
	{
		ShellCrossSection::Pointer& section = mSections[data.gpIndex];

		// Setup flag to compute ply constitutive matrices
		// (units [Pa] and rotated to element orientation)
		section->SetupGetPlyConstitutiveMatrices(data.shearStabilisation);
		CalculateSectionResponse(data);

		// Resize output vector. 2 Surfaces for each ply
		data.rlaminateStresses.resize(2 * section->NumberOfPlies());
		for (unsigned int i = 0; i < 2 * section->NumberOfPlies(); i++)
		{
			data.rlaminateStresses[i].resize(8, false);
			data.rlaminateStresses[i].clear();
		}

		// Loop over all plies - start from top ply, top surface
		for (unsigned int plyNumber = 0;
			plyNumber < section->NumberOfPlies(); ++plyNumber)
		{
			// determine stresses at currrent ply, top surface
			// (element coordinate system)
			data.rlaminateStresses[2 * plyNumber] = prod(
				section->GetPlyConstitutiveMatrix(plyNumber),
				data.rlaminateStrains[2 * plyNumber]);

			// determine stresses at currrent ply, bottom surface
			// (element coordinate system)
			data.rlaminateStresses[2 * plyNumber + 1] = prod(
				section->GetPlyConstitutiveMatrix(plyNumber),
				data.rlaminateStrains[2 * plyNumber + 1]);
		}
	}

	double ShellThickElement3D3N::CalculateTsaiWuPlaneStress(const CalculationData & data, const Matrix& rLamina_Strengths, const unsigned int& rPly)
	{
		// Incoming lamina strengths are organized as follows:
		// Refer to 'shell_cross_section.cpp' for details.
		//
		//	|	T1,		C1,		T2	|
		//	|	C2,		S12,	S13	|
		//	|   S23		0		0	|

		// Convert raw lamina strengths into tsai strengths F_i and F_ij.
		// Refer Reddy (2003) Section 10.9.4 (re-ordered for kratos DOFs).
		// All F_i3 components ignored - thin shell theory.
		//

		// Should be FALSE unless testing against other programs that ignore it.
		bool disable_in_plane_interaction = false;

		// First, F_i
		Vector F_i = Vector(3, 0.0);
		F_i[0] = 1.0 / rLamina_Strengths(0, 0) - 1.0 / rLamina_Strengths(0, 1);
		F_i[1] = 1.0 / rLamina_Strengths(0, 2) - 1.0 / rLamina_Strengths(1, 0);
		F_i[2] = 0.0;

		// Second, F_ij
		Matrix F_ij = Matrix(5, 5, 0.0);
		F_ij.clear();
		F_ij(0, 0) = 1.0 / rLamina_Strengths(0, 0) / rLamina_Strengths(0, 1);	// 11
		F_ij(1, 1) = 1.0 / rLamina_Strengths(0, 2) / rLamina_Strengths(1, 0);	// 22
		F_ij(2, 2) = 1.0 / rLamina_Strengths(1, 1) / rLamina_Strengths(1, 1);	// 12
		F_ij(0, 1) = F_ij(1, 0) = -0.5 / std::sqrt(rLamina_Strengths(0, 0)*rLamina_Strengths(0, 1)*rLamina_Strengths(0, 2)*rLamina_Strengths(1, 0));

		if (disable_in_plane_interaction)
		{
			F_ij(0, 1) = F_ij(1, 0) = 0.0;
		}

		// Third, addditional transverse shear terms
		F_ij(3, 3) = 1.0 / rLamina_Strengths(1, 2) / rLamina_Strengths(1, 2);	// 13
		F_ij(4, 4) = 1.0 / rLamina_Strengths(2, 0) / rLamina_Strengths(2, 0);	// 23

		// Evaluate Tsai-Wu @ top surface of current layer
		double var_a = 0.0;
		double var_b = 0.0;
		for (size_t i = 0; i < 3; i++)
		{
			var_b += F_i[i] * data.rlaminateStresses[2 * rPly][i];
			for (size_t j = 0; j < 3; j++)
			{
				var_a += F_ij(i, j)*data.rlaminateStresses[2 * rPly][i] * data.rlaminateStresses[2 * rPly][j];
			}
		}
		var_a += F_ij(3, 3)*data.rlaminateStresses[2 * rPly][6] * data.rlaminateStresses[2 * rPly][6]; // Transverse shear 13
		var_a += F_ij(4, 4)*data.rlaminateStresses[2 * rPly][7] * data.rlaminateStresses[2 * rPly][7]; // Transverse shear 23

		double tsai_reserve_factor_top = (-1.0*var_b + std::sqrt(var_b*var_b + 4.0 * var_a)) / 2.0 / var_a;

		// Evaluate Tsai-Wu @ bottom surface of current layer
		var_a = 0.0;
		var_b = 0.0;
		for (size_t i = 0; i < 3; i++)
		{
			var_b += F_i[i] * data.rlaminateStresses[2 * rPly + 1][i];
			for (size_t j = 0; j < 3; j++)
			{
				var_a += F_ij(i, j)*data.rlaminateStresses[2 * rPly + 1][i] * data.rlaminateStresses[2 * rPly + 1][j];
			}
		}
		var_a += F_ij(3, 3)*data.rlaminateStresses[2 * rPly + 1][6] * data.rlaminateStresses[2 * rPly + 1][6]; // Transverse shear 13
		var_a += F_ij(4, 4)*data.rlaminateStresses[2 * rPly + 1][7] * data.rlaminateStresses[2 * rPly + 1][7]; // Transverse shear 23

		double tsai_reserve_factor_bottom = (-1.0*var_b + std::sqrt(var_b*var_b + 4.0 * var_a)) / 2.0 / var_a;

		// Return min of both surfaces as the result for the whole ply
		return std::min(tsai_reserve_factor_bottom, tsai_reserve_factor_top);
	}

	void ShellThickElement3D3N::CheckGeneralizedStressOrStrainOutput(const Variable<Matrix>& rVariable, int & ijob, bool & bGlobal)
	{
		if (rVariable == SHELL_STRAIN)
		{
			ijob = 1;
		}
		else if (rVariable == SHELL_STRAIN_GLOBAL)
		{
			ijob = 1;
			bGlobal = true;
		}
		else if (rVariable == SHELL_CURVATURE)
		{
			ijob = 2;
		}
		else if (rVariable == SHELL_CURVATURE_GLOBAL)
		{
			ijob = 2;
			bGlobal = true;
		}
		else if (rVariable == SHELL_FORCE)
		{
			ijob = 3;
		}
		else if (rVariable == SHELL_FORCE_GLOBAL)
		{
			ijob = 3;
			bGlobal = true;
		}
		else if (rVariable == SHELL_MOMENT)
		{
			ijob = 4;
		}
		else if (rVariable == SHELL_MOMENT_GLOBAL)
		{
			ijob = 4;
			bGlobal = true;
		}
		else if (rVariable == SHELL_STRESS_TOP_SURFACE)
		{
			ijob = 5;
		}
		else if (rVariable == SHELL_STRESS_TOP_SURFACE_GLOBAL)
		{
			ijob = 5;
			bGlobal = true;
		}
		else if (rVariable == SHELL_STRESS_MIDDLE_SURFACE)
		{
			ijob = 6;
		}
		else if (rVariable == SHELL_STRESS_MIDDLE_SURFACE_GLOBAL)
		{
			ijob = 6;
			bGlobal = true;
		}
		else if (rVariable == SHELL_STRESS_BOTTOM_SURFACE)
		{
			ijob = 7;
		}
		else if (rVariable == SHELL_STRESS_BOTTOM_SURFACE_GLOBAL)
		{
			ijob = 7;
			bGlobal = true;
		}
		else if (rVariable == SHELL_ORTHOTROPIC_STRESS_BOTTOM_SURFACE)
		{
			ijob = 8;
		}
		else if (rVariable == SHELL_ORTHOTROPIC_STRESS_BOTTOM_SURFACE_GLOBAL)
		{
			ijob = 8;
			bGlobal = true;
		}
		else if (rVariable == SHELL_ORTHOTROPIC_STRESS_TOP_SURFACE)
		{
			ijob = 9;
		}
		else if (rVariable == SHELL_ORTHOTROPIC_STRESS_TOP_SURFACE_GLOBAL)
		{
			ijob = 9;
			bGlobal = true;
		}
		else if (rVariable == SHELL_ORTHOTROPIC_4PLY_THROUGH_THICKNESS)
		{
			// TESTING VARIABLE
			ijob = 99;
		}
	}

	void ShellThickElement3D3N::DecimalCorrection(Vector& a)
	{
		double norm = norm_2(a);
		double tolerance = std::max(norm * 1.0E-12, 1.0E-12);
		for (SizeType i = 0; i < a.size(); i++)
			if (std::abs(a(i)) < tolerance)
				a(i) = 0.0;
	}

	void ShellThickElement3D3N::SetupOrientationAngles()
	{
		ShellT3_LocalCoordinateSystem lcs(mpCoordinateTransformation->CreateReferenceCoordinateSystem());

		Vector3Type normal;
		noalias(normal) = lcs.Vz();

		Vector3Type dZ;
		dZ(0) = 0.0;
		dZ(1) = 0.0;
		dZ(2) = 1.0; // for the moment let's take this. But the user can specify its own triad! TODO

		Vector3Type dirX;
		MathUtils<double>::CrossProduct(dirX, dZ, normal);

		// try to normalize the x vector. if it is near zero it means that we need
		// to choose a default one.
		double dirX_norm = dirX(0)*dirX(0) + dirX(1)*dirX(1) + dirX(2)*dirX(2);
		if (dirX_norm < 1.0E-12)
		{
			dirX(0) = 1.0;
			dirX(1) = 0.0;
			dirX(2) = 0.0;
		}
		else if (dirX_norm != 1.0)
		{
			dirX_norm = std::sqrt(dirX_norm);
			dirX /= dirX_norm;
		}

		Vector3Type elem_dirX = lcs.Vx();

		// now calculate the angle between the element x direction and the material x direction.
		Vector3Type& a = elem_dirX;
		Vector3Type& b = dirX;
		double a_dot_b = a(0)*b(0) + a(1)*b(1) + a(2)*b(2);
		if (a_dot_b < -1.0) a_dot_b = -1.0;
		if (a_dot_b > 1.0) a_dot_b = 1.0;
		double angle = std::acos(a_dot_b);

		// if they are not counter-clock-wise, let's change the sign of the angle
		if (angle != 0.0)
		{
			const MatrixType& R = lcs.Orientation();
			if (dirX(0)*R(1, 0) + dirX(1)*R(1, 1) + dirX(2)*R(1, 2) < 0.0)
				angle = -angle;
		}

		for (CrossSectionContainerType::iterator it = mSections.begin(); it != mSections.end(); ++it)
			(*it)->SetOrientationAngle(angle);
	}

	void ShellThickElement3D3N::CalculateSectionResponse(CalculationData& data)
	{
		const array_1d<double, 3>& loc = data.gpLocations[0];
		data.N(0) = 1.0 - loc[1] - loc[2];
		data.N(1) = loc[1];
		data.N(2) = loc[2];

		ShellCrossSection::Pointer& section = mSections[0];
		data.SectionParameters.SetShapeFunctionsValues(data.N);
		section->CalculateSectionResponse(data.SectionParameters, ConstitutiveLaw::StressMeasure_PK2);

		if (data.basicTriCST == false &&
			data.ignore_shear_stabilization == false)
		{
			//std::cout << "applying shhear stbailization" << std::endl;
			//add in shear stabilization
			data.shearStabilisation = (data.hMean*data.hMean)
				/ (data.hMean*data.hMean + data.alpha*data.h_e*data.h_e);
			data.D(6, 6) *= data.shearStabilisation;
			data.D(6, 7) *= data.shearStabilisation;
			data.D(7, 6) *= data.shearStabilisation;
			data.D(7, 7) *= data.shearStabilisation;
		}
	}

	void ShellThickElement3D3N::InitializeCalculationData(CalculationData& data)
	{
		//-------------------------------------
		// Computation of all stuff that remain
		// constant throughout the calculations

		//-------------------------------------
		// geometry data

		const double x12 = data.LCS0.X1() - data.LCS0.X2();
		const double x23 = data.LCS0.X2() - data.LCS0.X3();
		const double x31 = data.LCS0.X3() - data.LCS0.X1();
		const double x21 = -x12;
		const double x32 = -x23;
		const double x13 = -x31;

		const double y12 = data.LCS0.Y1() - data.LCS0.Y2();
		const double y23 = data.LCS0.Y2() - data.LCS0.Y3();
		const double y31 = data.LCS0.Y3() - data.LCS0.Y1();
		const double y21 = -y12;

		const double y13 = -y31;

		const double A = 0.5*(y21*x13 - x21*y13);
		const double A2 = 2.0*A;

		double h = 0.0;
		for (unsigned int i = 0; i < mSections.size(); i++)
			h += mSections[i]->GetThickness();
		h /= (double)mSections.size();

		data.hMean = h;
		data.TotalArea = A;
		data.dA = A;

		// create the integration point locations
		if (data.gpLocations.size() != 0) data.gpLocations.clear();
		data.gpLocations.resize(OPT_NUM_GP);
#ifdef OPT_1_POINT_INTEGRATION
		array_1d<double, 3>& gp0 = data.gpLocations[0];
		gp0[0] = 1.0 / 3.0;
		gp0[1] = 1.0 / 3.0;
		gp0[2] = 1.0 / 3.0;
#else
		array_1d<double, 3>& gp0 = data.gpLocations[0];
		array_1d<double, 3>& gp1 = data.gpLocations[1];
		array_1d<double, 3>& gp2 = data.gpLocations[2];
#ifdef OPT_USES_INTERIOR_GAUSS_POINTS
		gp0[0] = 1.0 / 6.0;
		gp0[1] = 1.0 / 6.0;
		gp0[2] = 2.0 / 3.0;
		gp1[0] = 2.0 / 3.0;
		gp1[1] = 1.0 / 6.0;
		gp1[2] = 1.0 / 6.0;
		gp2[0] = 1.0 / 6.0;
		gp2[1] = 2.0 / 3.0;
		gp2[2] = 1.0 / 6.0;
#else
		gp0[0] = 0.5;
		gp0[1] = 0.5;
		gp0[2] = 0.0;
		gp1[0] = 0.0;
		gp1[1] = 0.5;
		gp1[2] = 0.5;
		gp2[0] = 0.5;
		gp2[1] = 0.0;
		gp2[2] = 0.5;
#endif // OPT_USES_INTERIOR_GAUSS_POINTS

#endif // OPT_1_POINT_INTEGRATION

		// cartesian derivatives
		data.dNxy(0, 0) = (y13 - y12) / A2;
		data.dNxy(0, 1) = (x12 - x13) / A2;
		data.dNxy(1, 0) = -y13 / A2;
		data.dNxy(1, 1) = x13 / A2;
		data.dNxy(2, 0) = y12 / A2;
		data.dNxy(2, 1) = -x12 / A2;

		// ---------------------------------------------------------------------
		// Total Formulation:
		//		as per Efficient Co-Rotational 3-Node Shell Element paper (2016)
		// ---------------------------------------------------------------------

		data.B.clear();

		//Membrane components
		//
		//node 1
		data.B(0, 0) = y23;
		data.B(1, 1) = x32;
		data.B(2, 0) = x32;
		data.B(2, 1) = y23;

		//node 2
		data.B(0, 6) = y31;
		data.B(1, 7) = x13;
		data.B(2, 6) = x13;
		data.B(2, 7) = y31;

		//node 3
		data.B(0, 12) = y12;
		data.B(1, 13) = x21;
		data.B(2, 12) = x21;
		data.B(2, 13) = y12;

		//Bending components
		//
		//node 1
		data.B(3, 4) = y23;
		data.B(4, 3) = -1.0 * x32;
		data.B(5, 3) = -1.0 * y23;
		data.B(5, 4) = x32;

		//node 2
		data.B(3, 10) = y31;
		data.B(4, 9) = -1.0 * x13;
		data.B(5, 9) = -1.0 * y31;
		data.B(5, 10) = x13;

		//node 3
		data.B(3, 16) = y12;
		data.B(4, 15) = -1.0 * x21;
		data.B(5, 15) = -1.0 * y12;
		data.B(5, 16) = x21;

		//Shear components
		//
		if (data.specialDSGc3)
		{
			// Don't do anything here.
			// The shear contribution will be added with 3GPs later on.
		}
		else if (data.basicTriCST == false)
		{
			// Use DSG method

			const double a = x21;
			const double b = y21;
			const double c = y31;
			const double d = x31;
			//node 1
			data.B(6, 2) = b - c;
			data.B(6, 4) = A;

			data.B(7, 2) = d - a;
			data.B(7, 3) = -1.0 * A;

			//node 2
			data.B(6, 8) = c;
			data.B(6, 9) = -1.0 * b*c / 2.0;
			data.B(6, 10) = a*c / 2.0;

			data.B(7, 8) = -1.0 * d;
			data.B(7, 9) = b*d / 2.0;
			data.B(7, 10) = -1.0 * a*d / 2.0;

			//node 3
			data.B(6, 14) = -1.0 * b;
			data.B(6, 15) = b*c / 2.0;
			data.B(6, 16) = b*d / 2.0;

			data.B(7, 14) = a;
			data.B(7, 15) = -1.0 * a*c / 2.0;
			data.B(7, 16) = a*d / 2.0;
		}
		else
		{
			// Basic CST displacement derived shear
			// strain displacement matrix.
			// Only for testing!

			const Matrix & shapeFunctions = 
				GetGeometry().ShapeFunctionsValues(mThisIntegrationMethod);

			//node 1
			data.B(6, 2) = y23;
			data.B(6, 3) = shapeFunctions(0, 0);

			data.B(7, 2) = x32;
			data.B(7, 4) = shapeFunctions(0, 0);

			//node 2
			data.B(6, 8) = y31;
			data.B(6, 9) = shapeFunctions(0, 1);

			data.B(7, 8) = x13;
			data.B(7, 10) = shapeFunctions(0, 1);

			//node 3
			data.B(6, 14) = y12;
			data.B(6, 15) = shapeFunctions(0, 2);

			data.B(7, 14) = x21;
			data.B(7, 16) = shapeFunctions(0, 2);
		}

		//Final multiplication
		data.B /= (A2);

		//determine longest side length (eqn 22) - for shear correction
		Vector P12 = Vector(data.LCS0.P1() - data.LCS0.P2());
		Vector P13 = Vector(data.LCS0.P1() - data.LCS0.P3());
		Vector P23 = Vector(data.LCS0.P2() - data.LCS0.P3());
		data.h_e = std::sqrt(inner_prod(P12, P12));
		double edge_length = std::sqrt(inner_prod(P13, P13));
		if (edge_length > data.h_e) { data.h_e = edge_length; }
		edge_length = std::sqrt(inner_prod(P23, P23));
		if (edge_length > data.h_e) { data.h_e = edge_length; }

		//--------------------------------------
		// Calculate material matrices
		//

		//allocate and setup
		data.SectionParameters.SetElementGeometry(GetGeometry());
		data.SectionParameters.SetMaterialProperties(GetProperties());
		data.SectionParameters.SetProcessInfo(data.CurrentProcessInfo);
		data.SectionParameters.SetGeneralizedStrainVector(data.generalizedStrains);
		data.SectionParameters.SetGeneralizedStressVector(data.generalizedStresses);
		data.SectionParameters.SetConstitutiveMatrix(data.D);
		data.SectionParameters.SetShapeFunctionsDerivatives(data.dNxy);
		Flags& options = data.SectionParameters.GetOptions();
		//options.Set(ConstitutiveLaw::COMPUTE_STRESS, data.CalculateRHS); //set
		// to false so we use the shear stabilization added in
		// 'CalculateSectionResponse()'
		options.Set(ConstitutiveLaw::COMPUTE_STRESS, false); //set to false
		options.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR,
			data.CalculateLHS);

		//--------------------------------------
		// calculate the displacement vector
		// in global and local coordinate systems

		data.globalDisplacements.clear();
		GetValuesVector(data.globalDisplacements);

		data.localDisplacements =
			mpCoordinateTransformation->CalculateLocalDisplacements(
				data.LCS, data.globalDisplacements);
	}

	void ShellThickElement3D3N::CalculateDSGc3Contribution(CalculationData & data, MatrixType & rLeftHandSideMatrix)
	{
		CalculateDSGc3AnsatzCoefficients(data);

		const double x1 = data.LCS0.X1();
		const double y1 = data.LCS0.Y1();
		const double x2 = data.LCS0.X2();
		const double y2 = data.LCS0.Y2();
		const double x3 = data.LCS0.X3();
		const double y3 = data.LCS0.Y3();


		// Material matrix data.D is already multiplied with A!!!
		//const double dA = data.TotalArea / 3.0;
		data.D /= 3.0;	// Corresponds to |J| = 2A with w_i = 1/6
		
		Matrix SuperB = Matrix(2, 9, 0.0);
		double xp = 0.0;
		double yp = 0.0;
		double loc1, loc2, loc3;
		for (size_t gauss_point = 0; gauss_point < 3; gauss_point++)
		{
			loc1 = data.gpLocations[gauss_point][0];
			loc2 = data.gpLocations[gauss_point][1];
			loc3 = data.gpLocations[gauss_point][2];

			xp = loc3*x1 + loc1*x2 + loc2*x3;
			yp = loc3*y1 + loc1*y2 + loc2*y3;

			//xp = loc1;
			//yp = loc2;

			SuperB.clear();
			for (size_t col = 0; col < 9; col++)
			{
				SuperB(0, col) = data.a8[col] + yp*(0.5*data.a5[col] - 0.5*data.a6[col]) - yp*(data.a8[col] + data.a9[col]);
				SuperB(0, col) = data.a9[col] - xp*(0.5*data.a5[col] - 0.5*data.a6[col]) - xp*(data.a8[col] + data.a9[col]);
			}
			SuperB.clear();

			// B mat from maple worksheet, with bubble mode
			SuperB(0, 0) = -1.0 + 2.0*loc2;
			SuperB(0, 1) = 1.0 - loc2;
			SuperB(0, 2) = -1.0*loc2;
			SuperB(0, 3) = 0.5 - loc2;
			SuperB(0, 4) = 0.5 - 0.5*loc2;
			SuperB(0, 5) = 0.5*loc2;
			SuperB(0, 6) = 0.0;
			SuperB(0, 7) = -0.5*loc2;
			SuperB(0, 8) = -0.5*loc2;

			SuperB(1, 0) = -1.0 + 2.0*loc1;
			SuperB(1, 1) = -1.0*loc1;
			SuperB(1, 2) = 1.0 - loc1;
			SuperB(1, 3) = 0.0;
			SuperB(1, 4) = -0.5*loc1;
			SuperB(1, 5) = -0.5*loc1;
			SuperB(1, 6) = 0.5 - loc1;
			SuperB(1, 7) = 0.5*loc1;
			SuperB(1, 8) = 0.5-0.5*loc1;

			SuperB.clear();
			// B mat from python with no bubble mode
			SuperB(0, 0) = -1.0;
			SuperB(0, 1) = 1.0;
			SuperB(0, 2) = 0.0;
			SuperB(0, 3) = -0.5*loc2 + 0.5;
			SuperB(0, 4) = 0.5;
			SuperB(0, 5) = 0.5*loc2;
			SuperB(0, 6) = 0.5*loc2;
			SuperB(0, 7) = -0.5*loc2;
			SuperB(0, 8) = 0.0;

			SuperB(1, 0) = -1.0;
			SuperB(1, 1) = 0.0;
			SuperB(1, 2) = 1.0;
			SuperB(1, 3) = 0.5*loc1;
			SuperB(1, 4) = 0.0;
			SuperB(1, 5) = -0.5*loc1;
			SuperB(1, 6) = 0.5 - 0.5*loc1;
			SuperB(1, 7) = 0.5*loc1;
			SuperB(1, 8) = 0.5;

			data.B.clear();
			// Transfer from Bletzinger B matrix to Kratos B matrix
			// Dofs from [w1, w2, w3, px1, ...] to [w1, px1, py1, w2, ...]

			for (size_t i = 0; i < 3; i++)
			{
				data.B(6, 2 + 6 * i) = SuperB(0, i);
				data.B(6, 3 + 6 * i) = SuperB(0, 3 + i);
				data.B(6, 4 + 6 * i) = SuperB(0, 6 + i);

				data.B(7, 2 + 6 * i) = SuperB(1, i);
				data.B(7, 3 + 6 * i) = SuperB(1, 3 + i);
				data.B(7, 4 + 6 * i) = SuperB(1, 6 + i);


			}

			Matrix temp = Matrix(prod(trans(data.B), data.D));
			rLeftHandSideMatrix += prod(temp, data.B);
		}		
	}

	void ShellThickElement3D3N::CalculateDSGc3AnsatzCoefficients(CalculationData & data)
	{
		const double x1 = data.LCS0.X1();
		const double y1 = data.LCS0.Y1();
		const double x2 = data.LCS0.X2();
		const double y2 = data.LCS0.Y2();
		const double x3 = data.LCS0.X3();
		const double y3 = data.LCS0.Y3();

		data.a5[3] = -(x2 - x3) / (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2);
		data.a5[4] = (x1 - x3) / (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2);
		data.a5[5] = -(x1 - x2) / (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2);

		data.a6[6] = (y2 - y3) / (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2);
		data.a6[7] = -(y1 - y3) / (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2);
		data.a6[8] = (y1 - y2) / (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2);

		data.a8[0] = -(x2*y2 - x3*y3 - y2 + y3) / (x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2);
		data.a8[1] = (x1*y1 - x3*y3 - y1 + y3) / (x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2);
		data.a8[2] = -(x1*y1 - x2*y2 - y1 + y2) / (x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2);
		data.a8[3] = -(x1*x1 * x2*y2*y2 - x1*x1 * x2*y2*y3 - x1*x1 * x3*y2*y3 + x1*x1 * x3*y3*y3 - x1*x1 * y2*y2 + 2 * x1*x1 * y2*y3 - x1*x1 * y3*y3 - x1*x2*x2 * y1*y2 - x1*x2*x2 * y1*y3 + 2 * x1*x2*x2 * y2*y3 + 2 * x1*x2*x3*y1*y2 + 2 * x1*x2*x3*y1*y3 - 2 * x1*x2*x3*y2*y2 - 2 * x1*x2*x3*y3*y3 + x1*x2*y1*y2 - x1*x2*y1*y3 - 2 * x1*x2*y2*y3 + 2 * x1*x2*y3*y3 - x1*x3*x3 * y1*y2 - x1*x3*x3 * y1*y3 + 2 * x1*x3*x3 * y2*y3 - x1*x3*y1*y2 + x1*x3*y1*y3 + 2 * x1*x3*y2*y2 - 2 * x1*x3*y2*y3 - x2*x2 * x3*y2*y3 + x2*x2 * x3*y3*y3 + x2*x2 * y1*y3 - x2*x2 * y3*y3 + x2*x3*x3 * y2*y2 - x2*x3*x3 * y2*y3 - x2*x3*y1*y2 - x2*x3*y1*y3 + 2 * x2*x3*y2*y3 + x3*x3 * y1*y2 - x3*x3 * y2*y2) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a8[4] = (x1*x1 * x2*y1*y2 - 2 * x1*x1 * x2*y1*y3 + x1*x1 * x2*y2*y3 + x1*x1 * x3*y1*y3 - x1*x1 * x3*y3*y3 - x1*x1 * y2*y3 + x1*x1 * y3*y3 - x1*x2*x2 * y1*y1 + x1*x2*x2 * y1*y3 + 2 * x1*x2*x3*y1*y1 - 2 * x1*x2*x3*y1*y2 - 2 * x1*x2*x3*y2*y3 + 2 * x1*x2*x3*y3*y3 - x1*x2*y1*y2 + 2 * x1*x2*y1*y3 + x1*x2*y2*y3 - 2 * x1*x2*y3*y3 - x1*x3*x3 * y1*y1 + x1*x3*x3 * y1*y3 + x1*x3*y1*y2 - 2 * x1*x3*y1*y3 + x1*x3*y2*y3 + x2*x2 * x3*y1*y3 - x2*x2 * x3*y3*y3 + x2*x2 * y1*y1 - 2 * x2*x2 * y1*y3 + x2*x2 * y3*y3 + x2*x3*x3 * y1*y2 - 2 * x2*x3*x3 * y1*y3 + x2*x3*x3 * y2*y3 - 2 * x2*x3*y1*y1 + x2*x3*y1*y2 + 2 * x2*x3*y1*y3 - x2*x3*y2*y3 + x3*x3 * y1*y1 - x3*x3 * y1*y2) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a8[5] = (x1*x1 * x2*y1*y2 - x1*x1 * x2*y2*y2 - 2 * x1*x1 * x3*y1*y2 + x1*x1 * x3*y1*y3 + x1*x1 * x3*y2*y3 + x1*x1 * y2*y2 - x1*x1 * y2*y3 - x1*x2*x2 * y1*y1 + x1*x2*x2 * y1*y2 + 2 * x1*x2*x3*y1*y1 - 2 * x1*x2*x3*y1*y3 + 2 * x1*x2*x3*y2*y2 - 2 * x1*x2*x3*y2*y3 - 2 * x1*x2*y1*y2 + x1*x2*y1*y3 + x1*x2*y2*y3 - x1*x3*x3 * y1*y1 + x1*x3*x3 * y1*y2 + 2 * x1*x3*y1*y2 - x1*x3*y1*y3 - 2 * x1*x3*y2*y2 + x1*x3*y2*y3 - 2 * x2*x2 * x3*y1*y2 + x2*x2 * x3*y1*y3 + x2*x2 * x3*y2*y3 + x2*x2 * y1*y1 - x2*x2 * y1*y3 + x2*x3*x3 * y1*y2 - x2*x3*x3 * y2*y2 - 2 * x2*x3*y1*y1 + 2 * x2*x3*y1*y2 + x2*x3*y1*y3 - x2*x3*y2*y3 + x3*x3 * y1*y1 - 2 * x3*x3 * y1*y2 + x3*x3 * y2*y2) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a8[6] = -(x1*x2*y1*y2*y2 - 2 * x1*x2*y1*y2*y3 + x1*x2*y1*y3*y3 + x1*x3*y1*y2*y2 - 2 * x1*x3*y1*y2*y3 + x1*x3*y1*y3*y3 - x1*y1*y2*y2 + 2 * x1*y1*y2*y3 - x1*y1*y3*y3 - x2*x2 * y1*y1 * y2 + 2 * x2*x2 * y1*y2*y3 - x2*x2 * y2*y3*y3 + x2*x3*y1*y1 * y2 + x2*x3*y1*y1 * y3 - 2 * x2*x3*y1*y2*y2 - 2 * x2*x3*y1*y3*y3 + x2*x3*y2*y2 * y3 + x2*x3*y2*y3*y3 + x2*y1*y1 * y2 - x2*y1*y1 * y3 - x2*y1*y2*y3 + x2*y1*y3*y3 - x3*x3 * y1*y1 * y3 + 2 * x3*x3 * y1*y2*y3 - x3*x3 * y2*y2 * y3 - x3*y1*y1 * y2 + x3*y1*y1 * y3 + x3*y1*y2*y2 - x3*y1*y2*y3) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a8[7] = (x1*x1 * y1*y2*y2 - 2 * x1*x1 * y1*y2*y3 + x1*x1 * y1*y3*y3 - x1*x2*y1*y1 * y2 + 2 * x1*x2*y1*y2*y3 - x1*x2*y2*y3*y3 + 2 * x1*x3*y1*y1 * y2 - x1*x3*y1*y1 * y3 - x1*x3*y1*y2*y2 - x1*x3*y1*y3*y3 - x1*x3*y2*y2 * y3 + 2 * x1*x3*y2*y3*y3 - x1*y1*y2*y2 + x1*y1*y2*y3 + x1*y2*y2 * y3 - x1*y2*y3*y3 - x2*x3*y1*y1 * y2 + 2 * x2*x3*y1*y2*y3 - x2*x3*y2*y3*y3 + x2*y1*y1 * y2 - 2 * x2*y1*y2*y3 + x2*y2*y3*y3 + x3*x3 * y1*y1 * y3 - 2 * x3*x3 * y1*y2*y3 + x3*x3 * y2*y2 * y3 - x3*y1*y1 * y2 + x3*y1*y2*y2 + x3*y1*y2*y3 - x3*y2*y2 * y3) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a8[8] = (x1*x1 * y1*y2*y2 - 2 * x1*x1 * y1*y2*y3 + x1*x1 * y1*y3*y3 - x1*x2*y1*y1 * y2 + 2 * x1*x2*y1*y1 * y3 - x1*x2*y1*y2*y2 - x1*x2*y1*y3*y3 + 2 * x1*x2*y2*y2 * y3 - x1*x2*y2*y3*y3 - x1*x3*y1*y1 * y3 + 2 * x1*x3*y1*y2*y3 - x1*x3*y2*y2 * y3 + x1*y1*y2*y3 - x1*y1*y3*y3 - x1*y2*y2 * y3 + x1*y2*y3*y3 + x2*x2 * y1*y1 * y2 - 2 * x2*x2 * y1*y2*y3 + x2*x2 * y2*y3*y3 - x2*x3*y1*y1 * y3 + 2 * x2*x3*y1*y2*y3 - x2*x3*y2*y2 * y3 - x2*y1*y1 * y3 + x2*y1*y2*y3 + x2*y1*y3*y3 - x2*y2*y3*y3 + x3*y1*y1 * y3 - 2 * x3*y1*y2*y3 + x3*y2*y2 * y3) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));


		data.a9[0] = (x2*y2 - x2 - x3*y3 + x3) / (x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2);
		data.a9[1] = -(x1*y1 - x1 - x3*y3 + x3) / (x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2);
		data.a9[2] = (x1*y1 - x1 - x2*y2 + x2) / (x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2);
		data.a9[3] = (x1*x1 * x2*y2*y2 - x1*x1 * x2*y2*y3 - x1*x1 * x2*y2 + x1*x1 * x2*y3 - x1*x1 * x3*y2*y3 + x1*x1 * x3*y2 + x1*x1 * x3*y3*y3 - x1*x1 * x3*y3 - x1*x2*x2 * y1*y2 - x1*x2*x2 * y1*y3 + x1*x2*x2 * y1 + 2 * x1*x2*x2 * y2*y3 - x1*x2*x2 * y3 + 2 * x1*x2*x3*y1*y2 + 2 * x1*x2*x3*y1*y3 - 2 * x1*x2*x3*y1 - 2 * x1*x2*x3*y2*y2 + x1*x2*x3*y2 - 2 * x1*x2*x3*y3*y3 + x1*x2*x3*y3 - x1*x3*x3 * y1*y2 - x1*x3*x3 * y1*y3 + x1*x3*x3 * y1 + 2 * x1*x3*x3 * y2*y3 - x1*x3*x3 * y2 - x2*x2 * x3*y2*y3 + x2*x2 * x3*y3*y3 + x2*x3*x3 * y2*y2 - x2*x3*x3 * y2*y3) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a9[4] = -(x1*x1 * x2*y1*y2 - 2 * x1*x1 * x2*y1*y3 + x1*x1 * x2*y2*y3 - x1*x1 * x2*y2 + x1*x1 * x2*y3 + x1*x1 * x3*y1*y3 - x1*x1 * x3*y3*y3 - x1*x2*x2 * y1*y1 + x1*x2*x2 * y1*y3 + x1*x2*x2 * y1 - x1*x2*x2 * y3 + 2 * x1*x2*x3*y1*y1 - 2 * x1*x2*x3*y1*y2 - x1*x2*x3*y1 - 2 * x1*x2*x3*y2*y3 + 2 * x1*x2*x3*y2 + 2 * x1*x2*x3*y3*y3 - x1*x2*x3*y3 - x1*x3*x3 * y1*y1 + x1*x3*x3 * y1*y3 + x2*x2 * x3*y1*y3 - x2*x2 * x3*y1 - x2*x2 * x3*y3*y3 + x2*x2 * x3*y3 + x2*x3*x3 * y1*y2 - 2 * x2*x3*x3 * y1*y3 + x2*x3*x3 * y1 + x2*x3*x3 * y2*y3 - x2*x3*x3 * y2) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a9[5] = -(x1*x1 * x2*y1*y2 - x1*x1 * x2*y2*y2 - 2 * x1*x1 * x3*y1*y2 + x1*x1 * x3*y1*y3 + x1*x1 * x3*y2*y3 + x1*x1 * x3*y2 - x1*x1 * x3*y3 - x1*x2*x2 * y1*y1 + x1*x2*x2 * y1*y2 + 2 * x1*x2*x3*y1*y1 - 2 * x1*x2*x3*y1*y3 - x1*x2*x3*y1 + 2 * x1*x2*x3*y2*y2 - 2 * x1*x2*x3*y2*y3 - x1*x2*x3*y2 + 2 * x1*x2*x3*y3 - x1*x3*x3 * y1*y1 + x1*x3*x3 * y1*y2 + x1*x3*x3 * y1 - x1*x3*x3 * y2 - 2 * x2*x2 * x3*y1*y2 + x2*x2 * x3*y1*y3 + x2*x2 * x3*y1 + x2*x2 * x3*y2*y3 - x2*x2 * x3*y3 + x2*x3*x3 * y1*y2 - x2*x3*x3 * y1 - x2*x3*x3 * y2*y2 + x2*x3*x3 * y2) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a9[6] = (x1*x2*y1*y2*y2 - 2 * x1*x2*y1*y2*y3 - x1*x2*y1*y2 + x1*x2*y1*y3*y3 + x1*x2*y1*y3 + x1*x2*y2*y3 - x1*x2*y3*y3 + x1*x3*y1*y2*y2 - 2 * x1*x3*y1*y2*y3 + x1*x3*y1*y2 + x1*x3*y1*y3*y3 - x1*x3*y1*y3 - x1*x3*y2*y2 + x1*x3*y2*y3 - x2*x2 * y1*y1 * y2 + x2*x2 * y1*y1 + 2 * x2*x2 * y1*y2*y3 - 2 * x2*x2 * y1*y3 - x2*x2 * y2*y3*y3 + x2*x2 * y3*y3 + x2*x3*y1*y1 * y2 + x2*x3*y1*y1 * y3 - 2 * x2*x3*y1*y1 - 2 * x2*x3*y1*y2*y2 + 2 * x2*x3*y1*y2 - 2 * x2*x3*y1*y3*y3 + 2 * x2*x3*y1*y3 + x2*x3*y2*y2 * y3 + x2*x3*y2*y3*y3 - 2 * x2*x3*y2*y3 - x3*x3 * y1*y1 * y3 + x3*x3 * y1*y1 + 2 * x3*x3 * y1*y2*y3 - 2 * x3*x3 * y1*y2 - x3*x3 * y2*y2 * y3 + x3*x3 * y2*y2) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a9[7] = -(x1*x1 * y1*y2*y2 - 2 * x1*x1 * y1*y2*y3 + x1*x1 * y1*y3*y3 - x1*x1 * y2*y2 + 2 * x1*x1 * y2*y3 - x1*x1 * y3*y3 - x1*x2*y1*y1 * y2 + 2 * x1*x2*y1*y2*y3 + x1*x2*y1*y2 - x1*x2*y1*y3 - x1*x2*y2*y3*y3 - x1*x2*y2*y3 + x1*x2*y3*y3 + 2 * x1*x3*y1*y1 * y2 - x1*x3*y1*y1 * y3 - x1*x3*y1*y2*y2 - 2 * x1*x3*y1*y2 - x1*x3*y1*y3*y3 + 2 * x1*x3*y1*y3 - x1*x3*y2*y2 * y3 + 2 * x1*x3*y2*y2 + 2 * x1*x3*y2*y3*y3 - 2 * x1*x3*y2*y3 - x2*x3*y1*y1 * y2 + x2*x3*y1*y1 + 2 * x2*x3*y1*y2*y3 - x2*x3*y1*y2 - x2*x3*y1*y3 - x2*x3*y2*y3*y3 + x2*x3*y2*y3 + x3*x3 * y1*y1 * y3 - x3*x3 * y1*y1 - 2 * x3*x3 * y1*y2*y3 + 2 * x3*x3 * y1*y2 + x3*x3 * y2*y2 * y3 - x3*x3 * y2*y2) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));
		data.a9[8] = -(x1*x1 * y1*y2*y2 - 2 * x1*x1 * y1*y2*y3 + x1*x1 * y1*y3*y3 - x1*x1 * y2*y2 + 2 * x1*x1 * y2*y3 - x1*x1 * y3*y3 - x1*x2*y1*y1 * y2 + 2 * x1*x2*y1*y1 * y3 - x1*x2*y1*y2*y2 + 2 * x1*x2*y1*y2 - x1*x2*y1*y3*y3 - 2 * x1*x2*y1*y3 + 2 * x1*x2*y2*y2 * y3 - x1*x2*y2*y3*y3 - 2 * x1*x2*y2*y3 + 2 * x1*x2*y3*y3 - x1*x3*y1*y1 * y3 + 2 * x1*x3*y1*y2*y3 - x1*x3*y1*y2 + x1*x3*y1*y3 - x1*x3*y2*y2 * y3 + x1*x3*y2*y2 - x1*x3*y2*y3 + x2*x2 * y1*y1 * y2 - x2*x2 * y1*y1 - 2 * x2*x2 * y1*y2*y3 + 2 * x2*x2 * y1*y3 + x2*x2 * y2*y3*y3 - x2*x2 * y3*y3 - x2*x3*y1*y1 * y3 + x2*x3*y1*y1 + 2 * x2*x3*y1*y2*y3 - x2*x3*y1*y2 - x2*x3*y1*y3 - x2*x3*y2*y2 * y3 + x2*x3*y2*y3) / (2 * (x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2)*(x1*x2*y1 - x1*x2*y2 - x1*x3*y1 + x1*x3*y3 - x1*y1*y2 + x1*y1*y3 + x1*y2 - x1*y3 + x2*x3*y2 - x2*x3*y3 + x2*y1*y2 - x2*y1 - x2*y2*y3 + x2*y3 - x3*y1*y3 + x3*y1 + x3*y2*y3 - x3*y2));

	}

	void ShellThickElement3D3N::AddBodyForces(CalculationData& data, VectorType& rRightHandSideVector)
	{
		// TODO p2 update this when results is sorted out
		// This is hardcoded to use 1 gauss point, despite the declared def
		// using 3 gps.
		const GeometryType& geom = GetGeometry();

		// Get shape functions
#ifdef OPT_USES_INTERIOR_GAUSS_POINTS
		const Matrix & N = GetGeometry().ShapeFunctionsValues(mThisIntegrationMethod);
#else
		// Disabled to use 1 gp below
		/*
		Matrix N(3, 3);
		for (unsigned int igauss = 0; igauss < OPT_NUM_GP; igauss++)
		{
			const array_1d<double, 3>& loc = data.gpLocations[igauss];
			N(igauss, 0) = 1.0 - loc[1] - loc[2];
			N(igauss, 1) = loc[1];
			N(igauss, 2) = loc[2];
		}
		*/
		// 1 gp used!
		Matrix N(1, 3);
		N(0, 0) = 1.0 / 3.0;
		N(0, 1) = 1.0 / 3.0;
		N(0, 2) = 1.0 / 3.0;

#endif // !OPT_USES_INTERIOR_GAUSS_POINTS

		// auxiliary
		array_1d<double, 3> bf;

		// gauss loop to integrate the external force vector
		//for (unsigned int igauss = 0; igauss < OPT_NUM_GP; igauss++)
		for (unsigned int igauss = 0; igauss < 1; igauss++)
		{
			// get mass per unit area
			double mass_per_unit_area = mSections[igauss]->CalculateMassPerUnitArea();

			// interpolate nodal volume accelerations to this gauss point
			// and obtain the body force vector
			bf.clear();
			for (unsigned int inode = 0; inode < 3; inode++)
			{
				if (geom[inode].SolutionStepsDataHas(VOLUME_ACCELERATION))
				{
					bf += N(igauss, inode) * geom[inode].FastGetSolutionStepValue(VOLUME_ACCELERATION);
				}
			}
			bf *= (mass_per_unit_area * data.dA);

			// add it to the RHS vector
			for (unsigned int inode = 0; inode < 3; inode++)
			{
				unsigned int index = inode * 6;
				double iN = N(igauss, inode);
				rRightHandSideVector[index + 0] += iN * bf[0];
				rRightHandSideVector[index + 1] += iN * bf[1];
				rRightHandSideVector[index + 2] += iN * bf[2];
			}
		}
	}

	void ShellThickElement3D3N::CalculateAll(MatrixType& rLeftHandSideMatrix,
		VectorType& rRightHandSideVector,
		ProcessInfo& rCurrentProcessInfo,
		const bool LHSrequired,
		const bool RHSrequired)
	{
		KRATOS_TRY
			// Resize the Left Hand Side if necessary,
			// and initialize it to Zero

			if ((rLeftHandSideMatrix.size1() != OPT_NUM_DOFS) || (rLeftHandSideMatrix.size2() != OPT_NUM_DOFS))
				rLeftHandSideMatrix.resize(OPT_NUM_DOFS, OPT_NUM_DOFS, false);
		noalias(rLeftHandSideMatrix) = ZeroMatrix(OPT_NUM_DOFS, OPT_NUM_DOFS);

		// Resize the Right Hand Side if necessary,
		// and initialize it to Zero

		if (rRightHandSideVector.size() != OPT_NUM_DOFS)
			rRightHandSideVector.resize(OPT_NUM_DOFS, false);
		noalias(rRightHandSideVector) = ZeroVector(OPT_NUM_DOFS);

		// Initialize common calculation variables
		CalculationData data(mpCoordinateTransformation, rCurrentProcessInfo);
		data.CalculateLHS = LHSrequired;
		data.CalculateRHS = RHSrequired;
		InitializeCalculationData(data);
		CalculateSectionResponse(data);

		// Calulate element stiffness
		Matrix BTD = Matrix(18, 8, 0.0);
		data.D *= data.TotalArea;
		BTD = prod(trans(data.B), data.D);
		noalias(rLeftHandSideMatrix) += prod(BTD, data.B);
		if (data.specialDSGc3)
		{
			CalculateDSGc3Contribution(data, rLeftHandSideMatrix);
		}

		//add in z_rot artificial stiffness
		double z_rot_multiplier = 0.001;
		double max_stiff = 0.0; //max diagonal stiffness
		for (int i = 0; i < 18; i++)
		{
			if (rLeftHandSideMatrix(i, i) > max_stiff)
			{
				max_stiff = rLeftHandSideMatrix(i, i);
			}
		}
		for (int i = 0; i < 3; i++)
		{
			rLeftHandSideMatrix(6 * i + 5, 6 * i + 5) =
				z_rot_multiplier*max_stiff;
		}

		// Add RHS term
		rRightHandSideVector -= prod(rLeftHandSideMatrix, data.localDisplacements);

		// Let the CoordinateTransformation finalize the calculation.
		// This will handle the transformation of the local matrices/vectors to
		// the global coordinate system.
		mpCoordinateTransformation->FinalizeCalculations(data.LCS,
			data.globalDisplacements,
			data.localDisplacements,
			rLeftHandSideMatrix,
			rRightHandSideVector,
			RHSrequired,
			LHSrequired);

		// Add body forces contributions. This doesn't depend on the coordinate system
		AddBodyForces(data, rRightHandSideVector);
		KRATOS_CATCH("")
	}

	bool ShellThickElement3D3N::TryGetValueOnIntegrationPoints_MaterialOrientation(const Variable<array_1d<double, 3> >& rVariable,
		std::vector<array_1d<double, 3> >& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
		// Check the required output

		int ijob = 0;
		if (rVariable == MATERIAL_ORIENTATION_DX)
			ijob = 1;
		else if (rVariable == MATERIAL_ORIENTATION_DY)
			ijob = 2;
		else if (rVariable == MATERIAL_ORIENTATION_DZ)
			ijob = 3;

		// quick return

		if (ijob == 0) return false;

		// resize output

		if (rValues.size() != OPT_NUM_GP)
			rValues.resize(OPT_NUM_GP);

		// Compute the local coordinate system.

		ShellT3_LocalCoordinateSystem localCoordinateSystem(
			mpCoordinateTransformation->CreateLocalCoordinateSystem());

		Vector3Type eZ = localCoordinateSystem.Vz();

		// Gauss Loop

		if (ijob == 1)
		{
			Vector3Type eX = localCoordinateSystem.Vx();
			for (int i = 0; i < OPT_NUM_GP; i++)
			{
				QuaternionType q = QuaternionType::FromAxisAngle(eZ(0), eZ(1), eZ(2), mSections[i]->GetOrientationAngle());
				q.RotateVector3(eX, rValues[i]);
			}
		}
		else if (ijob == 2)
		{
			Vector3Type eY = localCoordinateSystem.Vy();
			for (int i = 0; i < OPT_NUM_GP; i++)
			{
				QuaternionType q = QuaternionType::FromAxisAngle(eZ(0), eZ(1), eZ(2), mSections[i]->GetOrientationAngle());
				q.RotateVector3(eY, rValues[i]);
			}
		}
		else if (ijob == 3)
		{
			for (int i = 0; i < OPT_NUM_GP; i++)
			{
				noalias(rValues[i]) = eZ;
			}
		}

		return true;
	}

	bool ShellThickElement3D3N::TryGetValueOnIntegrationPoints_GeneralizedStrainsOrStresses(const Variable<Matrix>& rVariable,
		std::vector<Matrix>& rValues,
		const ProcessInfo& rCurrentProcessInfo)
	{
		// Check the required output
		int ijob = 0;
		bool bGlobal = false;
		CheckGeneralizedStressOrStrainOutput(rVariable, ijob, bGlobal);

		// quick return
		if (ijob == 0) return false;

		// resize output
		if (rValues.size() != OPT_NUM_GP)
			rValues.resize(OPT_NUM_GP);

		// Just to store the rotation matrix for visualization purposes
		Matrix R(8, 8);
		Matrix aux33(3, 3);

		// Initialize common calculation variables
		CalculationData data(mpCoordinateTransformation, rCurrentProcessInfo);
		if (ijob > 2)
		{
			data.CalculateLHS = true; // calc constitutive mat for forces/moments
		}
		else
		{
			data.CalculateLHS = false;
		}
		data.CalculateRHS = true;
		InitializeCalculationData(data);

		// Get the current displacements in global coordinate system and 
		// transform to reference local system
		ShellT3_LocalCoordinateSystem referenceCoordinateSystem(
			mpCoordinateTransformation->CreateReferenceCoordinateSystem());
		MatrixType Rdisp(18, 18);
		referenceCoordinateSystem.ComputeTotalRotationMatrix(Rdisp);
		data.localDisplacements = prod(Rdisp, data.globalDisplacements);

		// set the current integration point index
		data.gpIndex = 0;
		ShellCrossSection::Pointer& section = mSections[0];

		// compute strains
		noalias(data.generalizedStrains) = prod(data.B, data.localDisplacements);

		//compute forces
		if (ijob > 2)
		{
			if (ijob > 7)
			{
				//Calculate lamina stresses
				CalculateLaminaStrains(data);
				CalculateLaminaStresses(data);
			}
			else
			{
				CalculateSectionResponse(data);
				noalias(data.generalizedStresses) = prod(data.D, data.generalizedStrains);
				//TODO p3 might be able to achieve this function in shell_cross_section by modifying it a bit to include a shear stabilization flag

				if (ijob > 4)
				{
					// Compute stresses for isotropic materials
					CalculateStressesFromForceResultants(data.generalizedStresses,
						section->GetThickness());
				}
			}
			DecimalCorrection(data.generalizedStresses);
		}

		// adjust output
		DecimalCorrection(data.generalizedStrains);

		// store the results, but first rotate them back to the section
		// coordinate system. we want to visualize the results in that system not
		// in the element one!
		if (section->GetOrientationAngle() != 0.0 && !bGlobal)
		{
			if (ijob > 7)
			{
				section->GetRotationMatrixForGeneralizedStresses(-(section->GetOrientationAngle()), R);
				for (unsigned int i = 0; i < data.rlaminateStresses.size(); i++)
				{
					data.rlaminateStresses[i] = prod(R, data.rlaminateStresses[i]);
				}

				// TODO p2 if statement here
				section->GetRotationMatrixForGeneralizedStrains(-(section->GetOrientationAngle()), R);
				for (unsigned int i = 0; i < data.rlaminateStrains.size(); i++)
				{
					data.rlaminateStrains[i] = prod(R, data.rlaminateStrains[i]);
				}
			}
			else if (ijob > 2)
			{
				section->GetRotationMatrixForGeneralizedStresses(-(section->GetOrientationAngle()), R);
				data.generalizedStresses = prod(R, data.generalizedStresses);
			}
			else
			{
				section->GetRotationMatrixForGeneralizedStrains(-(section->GetOrientationAngle()), R);
				data.generalizedStrains = prod(R, data.generalizedStrains);
			}
		}

		// Gauss Loop.

		for (size_t i = 0; i < OPT_NUM_GP; i++)
		{
			// save results
			Matrix & iValue = rValues[i];
			if (iValue.size1() != 3 || iValue.size2() != 3)
				iValue.resize(3, 3, false);

			if (ijob == 1) // strains
			{
				iValue(0, 0) = data.generalizedStrains(0);
				iValue(1, 1) = data.generalizedStrains(1);
				iValue(2, 2) = 0.0;
				iValue(0, 1) = iValue(1, 0) = 0.5 * data.generalizedStrains(2);
				iValue(0, 2) = iValue(2, 0) = 0.5 * data.generalizedStrains(7);
				iValue(1, 2) = iValue(2, 1) = 0.5 * data.generalizedStrains(6);
			}
			else if (ijob == 2) // curvatures
			{
				iValue(0, 0) = data.generalizedStrains(3);
				iValue(1, 1) = data.generalizedStrains(4);
				iValue(2, 2) = 0.0;
				iValue(0, 1) = iValue(1, 0) = 0.5 * data.generalizedStrains(5);
				iValue(0, 2) = iValue(2, 0) = 0.0;
				iValue(1, 2) = iValue(2, 1) = 0.0;
			}
			else if (ijob == 3) // forces
			{
				iValue(0, 0) = data.generalizedStresses(0);
				iValue(1, 1) = data.generalizedStresses(1);
				iValue(2, 2) = 0.0;
				iValue(0, 1) = iValue(1, 0) = data.generalizedStresses(2);
				iValue(0, 2) = iValue(2, 0) = data.generalizedStresses(7);
				iValue(1, 2) = iValue(2, 1) = data.generalizedStresses(6);
			}
			else if (ijob == 4) // moments
			{
				iValue(0, 0) = data.generalizedStresses(3);
				iValue(1, 1) = data.generalizedStresses(4);
				iValue(2, 2) = 0.0;
				iValue(0, 1) = iValue(1, 0) = data.generalizedStresses(5);
				iValue(0, 2) = iValue(2, 0) = 0.0;
				iValue(1, 2) = iValue(2, 1) = 0.0;
				//std::cout << iValue << std::endl;
			}
			else if (ijob == 5) // SHELL_STRESS_TOP_SURFACE
			{
				iValue(0, 0) = data.generalizedStresses(0) +
					data.generalizedStresses(3);
				iValue(1, 1) = data.generalizedStresses(1) +
					data.generalizedStresses(4);
				iValue(2, 2) = 0.0;
				iValue(0, 1) = iValue(1, 0) = data.generalizedStresses[2] +
					data.generalizedStresses[5];
				iValue(0, 2) = iValue(2, 0) = 0.0;
				iValue(1, 2) = iValue(2, 1) = 0.0;
			}
			else if (ijob == 6) // SHELL_STRESS_MIDDLE_SURFACE
			{
				iValue(0, 0) = data.generalizedStresses(0);
				iValue(1, 1) = data.generalizedStresses(1);
				iValue(2, 2) = 0.0;
				iValue(0, 1) = iValue(1, 0) = data.generalizedStresses[2];
				iValue(0, 2) = iValue(2, 0) = data.generalizedStresses[6];
				iValue(1, 2) = iValue(2, 1) = data.generalizedStresses[7];
			}
			else if (ijob == 7) // SHELL_STRESS_BOTTOM_SURFACE
			{
				iValue(0, 0) = data.generalizedStresses(0) -
					data.generalizedStresses(3);
				iValue(1, 1) = data.generalizedStresses(1) -
					data.generalizedStresses(4);
				iValue(2, 2) = 0.0;
				iValue(0, 1) = iValue(1, 0) = data.generalizedStresses[2] -
					data.generalizedStresses[5];
				iValue(0, 2) = iValue(2, 0) = 0.0;
				iValue(1, 2) = iValue(2, 1) = 0.0;				
			}
			else if (ijob == 8) // SHELL_ORTHOTROPIC_STRESS_BOTTOM_SURFACE
			{
				iValue(0, 0) =
					data.rlaminateStresses[data.rlaminateStresses.size() - 1][0];
				iValue(1, 1) =
					data.rlaminateStresses[data.rlaminateStresses.size() - 1][1];
				iValue(2, 2) = 0.0;
				iValue(0, 1) = iValue(1, 0) =
					data.rlaminateStresses[data.rlaminateStresses.size() - 1][2];
				iValue(0, 2) = iValue(2, 0) =
					data.rlaminateStresses[data.rlaminateStresses.size() - 1][6];
				iValue(1, 2) = iValue(2, 1) =
					data.rlaminateStresses[data.rlaminateStresses.size() - 1][7];
			}
			else if (ijob == 9) // SHELL_ORTHOTROPIC_STRESS_TOP_SURFACE
			{
				iValue(0, 0) = data.rlaminateStresses[0][0];
				iValue(1, 1) = data.rlaminateStresses[0][1];
				iValue(2, 2) = 0.0;
				iValue(0, 1) = iValue(1, 0) = data.rlaminateStresses[0][2];
				iValue(0, 2) = iValue(2, 0) = data.rlaminateStresses[0][6];
				iValue(1, 2) = iValue(2, 1) = data.rlaminateStresses[0][7];
			}
			else if (ijob == 99) // SHELL_ORTHOTROPIC_4PLY_THROUGH_THICKNESS
			{
				// Testing variable to get lamina stress/strain values
				// on each surface of a 4 ply laminate

				int surface = 0; // start from top ply top surface
								 // Output global results sequentially
				for (size_t row = 0; row < 3; row++)
				{
					for (size_t col = 0; col < 3; col++)
					{
						if (surface > 7)
						{
							iValue(row, col) = 0.0;
						}
						else
						{
							iValue(row, col) = data.rlaminateStrains[surface][6];
						}
						surface++;
					}
				}


				bool tsai_wu_thru_output = false;
				if (tsai_wu_thru_output)
				{
					std::vector<Matrix> Laminae_Strengths =
						std::vector<Matrix>(section->NumberOfPlies());
					for (unsigned int ply = 0; ply < section->NumberOfPlies(); ply++)
					{
						Laminae_Strengths[ply].resize(3, 3, 0.0);
						Laminae_Strengths[ply].clear();
					}
					PropertiesType & props = GetProperties();
					section->GetLaminaeStrengths(Laminae_Strengths, props);
					Vector ply_orientation(section->NumberOfPlies());
					section->GetLaminaeOrientation(ply_orientation);

					CalculateLaminaStrains(data);
					CalculateLaminaStresses(data);

					// Rotate lamina stress from section CS 
					// to lamina angle to lamina material principal directions
					for (unsigned int ply = 0; ply < section->NumberOfPlies(); ply++)
					{
						double total_rotation = -ply_orientation[ply] - (section->GetOrientationAngle()); // already rotated to section CS
						section->GetRotationMatrixForGeneralizedStresses(total_rotation, R);
						//top surface of current ply
						data.rlaminateStresses[2 * ply] = prod(R, data.rlaminateStresses[2 * ply]);
						//bottom surface of current ply
						data.rlaminateStresses[2 * ply + 1] = prod(R, data.rlaminateStresses[2 * ply + 1]);
					}

					// Calculate Tsai-Wu criterion for each ply
					Vector tsai_output = Vector(9, 0.0);
					tsai_output.clear();
					double temp_tsai_wu = 0.0;
					for (unsigned int ply = 0; ply < section->NumberOfPlies(); ply++)
					{
						Vector lamina_stress_top = Vector(data.rlaminateStresses[2 * ply]);
						Vector lamina_stress_bottom = Vector(data.rlaminateStresses[2 * ply + 1]);

						// top surface
						data.rlaminateStresses[2 * ply + 1] = Vector(lamina_stress_top);
						temp_tsai_wu = CalculateTsaiWuPlaneStress(data, Laminae_Strengths[ply], ply);
						tsai_output[2 * ply] = temp_tsai_wu;

						// bottom surface
						data.rlaminateStresses[2 * ply] = Vector(lamina_stress_bottom);
						data.rlaminateStresses[2 * ply + 1] = Vector(lamina_stress_bottom);
						temp_tsai_wu = CalculateTsaiWuPlaneStress(data, Laminae_Strengths[ply], ply);
						tsai_output[2 * ply + 1] = temp_tsai_wu;
					}

					//dump into results

					int surface1 = 0; // start from top ply top surface
									 // Output global results sequentially
					for (size_t row = 0; row < 3; row++)
					{
						for (size_t col = 0; col < 3; col++)
						{
							if (surface1 > 7)
							{
								iValue(row, col) = 0.0;
							}
							else
							{
								iValue(row, col) = tsai_output[surface1];
							}
							surface1++;
						}
					}

				} // tsai wu output
			}

			// if requested, rotate the results in the global coordinate system
			if (bGlobal)
			{
				const Matrix& RG = data.LCS.Orientation();
				noalias(aux33) = prod(trans(RG), iValue);
				noalias(iValue) = prod(aux33, RG);
			}
		}

		OPT_INTERPOLATE_RESULTS_TO_STANDARD_GAUSS_POINTS(rValues);
		return true;
	}

	void ShellThickElement3D3N::printMatrix(Matrix& matrixIn, std::string stringIn)
	{
		std::cout << "\n" << stringIn << std::endl;
		for (unsigned i = 0; i < matrixIn.size1(); ++i)
		{
			std::cout << "| ";
			for (unsigned j = 0; j < matrixIn.size2(); ++j)
			{
				std::cout << std::fixed << std::setprecision(1) << std::setw(8) << matrixIn(i, j) << " | ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	// =====================================================================================
	//
	// Class ShellThickElement3D3N - Serialization
	//
	// =====================================================================================

	void ShellThickElement3D3N::save(Serializer& rSerializer) const
	{
		KRATOS_SERIALIZE_SAVE_BASE_CLASS(rSerializer, Element);
		rSerializer.save("CTr", mpCoordinateTransformation);
		rSerializer.save("Sec", mSections);
		rSerializer.save("IntM", (int)mThisIntegrationMethod);
	}

	void ShellThickElement3D3N::load(Serializer& rSerializer)
	{
		KRATOS_SERIALIZE_SAVE_BASE_CLASS(rSerializer, Element);
		rSerializer.load("CTr", mpCoordinateTransformation);
		rSerializer.load("Sec", mSections);
		int temp;
		rSerializer.load("IntM", temp);
		mThisIntegrationMethod = (IntegrationMethod)temp;
	}
}