/// \ingroup vtkWrappers
/// \class ttkDiscreteGradient
/// \author Your Name Here <Your Email Address Here>
/// \date The Date Here.
///
/// \brief TTK VTK-filter that wraps the discreteGradient processing package.
///
/// VTK wrapping code for the @DiscreteGradient package.
///
/// \param Input Input scalar field (vtkDataSet)
/// \param Output Output scalar field (vtkDataSet)
///
/// This filter can be used as any other VTK filter (for instance, by using the
/// sequence of calls SetInputData(), Update(), GetOutput()).
///
/// See the corresponding ParaView state file example for a usage example
/// within a VTK pipeline.
///
/// \sa ttk::DiscreteGradient
#ifndef _TTK_DISCRETEGRADIENT_H
#define _TTK_DISCRETEGRADIENT_H

// ttk code includes
#include<DiscreteGradient.h>
#include<ttkWrapper.h>

// VTK includes
#include<vtkCharArray.h>
#include<vtkDataArray.h>
#include<vtkDataSet.h>
#include<vtkDataSetAlgorithm.h>
#include<vtkDoubleArray.h>
#include<vtkFiltersCoreModule.h>
#include<vtkFloatArray.h>
#include<vtkInformation.h>
#include<vtkIntArray.h>
#include<vtkObjectFactory.h>
#include<vtkPointData.h>
#include<vtkCellData.h>
#include<vtkSmartPointer.h>
#include<vtkInformationVector.h>
#include<vtkLine.h>

class VTKFILTERSCORE_EXPORT ttkDiscreteGradient
: public vtkDataSetAlgorithm, public Wrapper{

  public:

    static ttkDiscreteGradient* New();

    vtkTypeMacro(ttkDiscreteGradient, vtkDataSetAlgorithm);

    // default ttk setters
    vtkSetMacro(debugLevel_, int);

    void SetThreadNumber(int threadNumber){
      ThreadNumber = threadNumber;
      SetThreads();
    }

    void SetUseAllCores(bool onOff){
      UseAllCores = onOff;
      SetThreads();
    }
    // end of default ttk setters

    vtkSetMacro(ScalarField, string);
    vtkGetMacro(ScalarField, string);

    vtkSetMacro(UseInputOffsetScalarField, int);
    vtkGetMacro(UseInputOffsetScalarField, int);

    vtkSetMacro(InputOffsetScalarFieldName, string);
    vtkGetMacro(InputOffsetScalarFieldName, string);

    vtkSetMacro(ReverseSaddleMaximumConnection, int);
    vtkGetMacro(ReverseSaddleMaximumConnection, int);

    vtkSetMacro(ReverseSaddleSaddleConnection, int);
    vtkGetMacro(ReverseSaddleSaddleConnection, int);

    vtkSetMacro(AllowSecondPass, int);
    vtkGetMacro(AllowSecondPass, int);

    vtkSetMacro(AllowThirdPass, int);
    vtkGetMacro(AllowThirdPass, int);

    vtkSetMacro(ComputeGradientGlyphs, int);
    vtkGetMacro(ComputeGradientGlyphs, int);

    vtkSetMacro(IterationThreshold, int);
    vtkGetMacro(IterationThreshold, int);

    vtkSetMacro(ScalarFieldId, int);
    vtkGetMacro(ScalarFieldId, int);

    vtkSetMacro(OffsetFieldId, int);
    vtkGetMacro(OffsetFieldId, int);

    int setupTriangulation(vtkDataSet* input);
    int getScalars(vtkDataSet* input);
    int getOffsets(vtkDataSet* input);

  protected:

    ttkDiscreteGradient();
    ~ttkDiscreteGradient();

    TTK_SETUP();

    int FillInputPortInformation(int port, vtkInformation* info);
    int FillOutputPortInformation(int port, vtkInformation* info);

  private:

    string ScalarField;
    string InputOffsetScalarFieldName;
    bool UseInputOffsetScalarField;
    bool ReverseSaddleMaximumConnection;
    bool ReverseSaddleSaddleConnection;
    bool AllowSecondPass;
    bool AllowThirdPass;
    bool ComputeGradientGlyphs;
    int IterationThreshold;
    int ScalarFieldId;
    int OffsetFieldId;

    Triangulation* triangulation_;
    DiscreteGradient discreteGradient_;
    vtkDataArray* inputScalars_;
    vtkIntArray* offsets_;
    vtkDataArray* inputOffsets_;
    bool hasUpdatedMesh_;
};

#endif // _TTK_DISCRETEGRADIENT_H