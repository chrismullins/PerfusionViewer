
#include <vtkImageReader2Factory.h>
#include <vtkImageReader2.h>
#include <vtkImageMapper3D.h>

#include <vtkBoxWidget.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCommand.h>
#include <vtkPlanes.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkGPUVolumeRayCastMapper.h>

#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkSphere.h>
#include <vtkSampleFunction.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolumeProperty.h>
#include <vtkCamera.h>
#include <vtkImageShiftScale.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkXMLImageDataReader.h>
#include <vtkPolyDataReader.h>
#include <vtkTransform.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkObjectFactory.h>

// Define interaction style
class MouseInteractorStyle4 : public vtkInteractorStyleTrackballCamera
{
  public:
    static MouseInteractorStyle4* New();
    vtkTypeMacro(MouseInteractorStyle4, vtkInteractorStyleTrackballCamera);
 
    virtual void OnLeftButtonDown() 
    {
      std::cout << "Pressed left mouse button." << std::endl;
      // Forward events
      vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
    }
 
    virtual void OnMiddleButtonDown() 
    {
      std::cout << "Pressed middle mouse button." << std::endl;
      // Forward events
      vtkInteractorStyleTrackballCamera::OnMiddleButtonDown();
    }
 
    virtual void OnRightButtonDown() 
    {
      std::cout << "Pressed right mouse button." << std::endl;
      // Forward events
      vtkInteractorStyleTrackballCamera::OnRightButtonDown();
    }
 
};

vtkStandardNewMacro(MouseInteractorStyle4);

class vtkMyCallback : public vtkCommand
{
public:
  vtkSmartVolumeMapper *rcm;
  static vtkMyCallback *New() 
    { return new vtkMyCallback; }
    virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
	   vtkBoxWidget *boxWidget = reinterpret_cast<vtkBoxWidget*>(caller);
	   vtkPlanes *planes = vtkPlanes::New();
	   // The implicit function vtkPlanes is used in conjunction with the
	   // volume ray cast mapper to limit which portion of the volume is
	   // volume rendered.
       //boxWidget->InsideOutOn();
	   boxWidget->GetPlanes(planes);
	   rcm->SetClippingPlanes(planes);
    }
};


int main(int argc, char *argv[])
{
  vtkSmartPointer<vtkImageData> imageData =
    vtkSmartPointer<vtkImageData>::New();
  vtkSmartPointer<vtkPolyData> labelData =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPolyDataReader> labelReader = 
      vtkSmartPointer<vtkPolyDataReader>::New();
  if (argc < 2)
    {
    //CreateImageData(imageData);
    std::cout << "No data specified!" << std::endl;
    }
  else
    {
    // Read file
    std::string inputFilename = argv[1];
    vtkSmartPointer<vtkImageReader2Factory> readerFactory =
      vtkSmartPointer<vtkImageReader2Factory>::New();
    vtkImageReader2 * imageReader = readerFactory->CreateImageReader2(inputFilename.c_str());
    imageReader->SetFileName(inputFilename.c_str());
    imageReader->Update();
    imageData->ShallowCopy(imageReader->GetOutput());

    std::string labelFilename = argv[2];
    labelReader->SetFileName(labelFilename.c_str());
    labelReader->Update();
    }

  // The perfusion data needs to be "blown up"
  // I used a factor of 10, but if this is incorrect it's easily fixed here.
  vtkSmartPointer<vtkTransform> perfusionTransform = 
      vtkSmartPointer<vtkTransform>::New();
  perfusionTransform->Scale(1.00, 1.00, 10.0);

  // The tumor ROI needs to be rotated around Z and also "blown up"
  // Once again, fix the scale here if necessary.
  vtkSmartPointer<vtkTransform> tumorTransform =
    vtkSmartPointer<vtkTransform>::New();
  tumorTransform->RotateZ(180);
  tumorTransform->Scale(1.00, 1.00, 10.00);
  
  vtkSmartPointer<vtkRenderWindow> renWin = 
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderer> ren1 = 
    vtkSmartPointer<vtkRenderer>::New();
  ren1->SetBackground(0.1,0.2,0.2);
  
  renWin->AddRenderer(ren1);
  
  renWin->SetSize(301,300); // intentional odd and NPOT  width/height
  
  vtkSmartPointer<vtkRenderWindowInteractor> iren = 
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  
  renWin->Render(); // make sure we have an OpenGL context.
  
  vtkSmartPointer<vtkSmartVolumeMapper> volumeMapper = 
    vtkSmartPointer<vtkSmartVolumeMapper>::New();
    volumeMapper->SetBlendModeToComposite(); // composite first
#if VTK_MAJOR_VERSION <= 5
  volumeMapper->SetInputConnection(imageData->GetProducerPort());
#else
  volumeMapper->SetInputData(imageData);
#endif  
  vtkSmartPointer<vtkVolumeProperty> volumeProperty = 
    vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkSmartPointer<vtkPolyDataMapper> labelMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  labelMapper->SetInputConnection(labelReader->GetOutputPort());
  std::cout << "number of labels: " << labelMapper->GetNumberOfPieces() << std::endl;
  labelMapper->SetScalarRange(0, 10);

  vtkSmartPointer<vtkActor> labelActor = 
    vtkSmartPointer<vtkActor>::New();
  labelActor->SetMapper(labelMapper);
  labelActor->SetUserTransform(tumorTransform);
  labelActor->GetProperty()->SetOpacity(0.5);

  vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity = 
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  compositeOpacity->AddPoint(0.0,0.0);
  compositeOpacity->AddPoint(80.0,1.0);
  compositeOpacity->AddPoint(80.1,0.0);
  compositeOpacity->AddPoint(255.0,0.0);
  volumeProperty->SetScalarOpacity(compositeOpacity); // composite first.
  
  vtkSmartPointer<vtkColorTransferFunction> color = 
    vtkSmartPointer<vtkColorTransferFunction>::New();
  color->AddRGBPoint(0.0  ,0.0,0.0,1.0);
  color->AddRGBPoint(40.0  ,1.0,0.0,0.0);
  color->AddRGBPoint(255.0,1.0,1.0,1.0);
  volumeProperty->SetColor(color);
    
  vtkSmartPointer<vtkVolume> volume = 
    vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  volume->SetUserTransform(perfusionTransform);
  ren1->AddViewProp(volume);
  ren1->AddActor(labelActor);
  ren1->ResetCamera();
  
  // Render composite. In default mode. For coverage.
  renWin->Render();
  
  // 3D texture mode. For coverage.
  //volumeMapper->SetRequestedRenderModeToRayCastAndTexture();
  renWin->Render();
  
  // Software mode, for coverage. It also makes sure we will get the same
  // regression image on all platforms.
  volumeMapper->SetRequestedRenderModeToRayCast();
  renWin->Render();

vtkSmartPointer<MouseInteractorStyle4> style =
    vtkSmartPointer<MouseInteractorStyle4>::New();
  iren->SetInteractorStyle(style);

vtkSmartPointer<vtkBoxWidget> boxWidget = 
    vtkSmartPointer<vtkBoxWidget>::New();
  boxWidget->SetInteractor(iren);
  boxWidget->SetPlaceFactor(1.25);
  boxWidget->SetDefaultRenderer(ren1);

  vtkMyCallback *mcb = vtkMyCallback::New();
  mcb->rcm=volumeMapper;
  boxWidget->AddObserver(vtkCommand::InteractionEvent, mcb);
  mcb->Delete();
  boxWidget->SetInputData(imageData);
  boxWidget->PlaceWidget();
  vtkPlanes *planes = vtkPlanes::New();
  //boxWidget->GetPlanes(planes);
  boxWidget->InsideOutOn();  
  
  boxWidget->On();
  

  iren->Start();

  
  
  return EXIT_SUCCESS;
}

