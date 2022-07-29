#include <iostream>
#include "vtkPolyDataReader.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkVRMLExporter.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkLookupTable.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkProperty.h"
#include "vtkImageData.h"
#include "vtkNIFTIImageReader.h"
#include "vtkImageGaussianSmooth.h"
#include "vtkPointData.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkThreshold.h"
#include "vtkMarchingCubes.h"
#include "vtkImageImport.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageFileReader.h"
#include "itkVTKImageExport.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "ColorTable.hxx"



template <class TImageType>
void ConnectITKExporterToVTKImporter(
    itk::VTKImageExport<TImageType> *exporter, vtkImageImport *importer,
    bool connect_origin = true, bool connect_spacing = true, bool connect_direction = false)
{
  importer->SetUpdateInformationCallback(
    exporter->GetUpdateInformationCallback());
  importer->SetPipelineModifiedCallback(
    exporter->GetPipelineModifiedCallback());
  importer->SetWholeExtentCallback(
    exporter->GetWholeExtentCallback());
  if(connect_spacing)
    importer->SetSpacingCallback(
      exporter->GetSpacingCallback());
  if(connect_origin)
    importer->SetOriginCallback(
      exporter->GetOriginCallback());
  if(connect_direction)
    importer->SetDirectionCallback(
       exporter->GetDirectionCallback());
  importer->SetScalarTypeCallback(
    exporter->GetScalarTypeCallback());
  importer->SetNumberOfComponentsCallback(
    exporter->GetNumberOfComponentsCallback());
  importer->SetPropagateUpdateExtentCallback(
    exporter->GetPropagateUpdateExtentCallback());
  importer->SetUpdateDataCallback(
    exporter->GetUpdateDataCallback());
  importer->SetDataExtentCallback(
    exporter->GetDataExtentCallback());
  importer->SetBufferPointerCallback(
    exporter->GetBufferPointerCallback());
  importer->SetCallbackUserData(
    exporter->GetCallbackUserData());
}


void usage(std::ostream &os)
{
  os << "Usage:" << std::endl << std::endl;
  os << "VRMLConverter -s -l labeldesc.txt input.nii.gz output.vrml" << std::endl << std::endl;
  os << "Place all options before input and output filenames" << std::endl;
  os << "-h                   Print usage" << std::endl;
  os << "-s                   Optional. Turn on smoothing. Smoothing is off if no -s" << std::endl;
  os << "-l labeldesc.txt     Optional. Use an ITK-SNAP Label Description file" << std::endl;
  os << "                     to define output label colors. If option or file not provided, " << std::endl;
  os << "                     program will use the default ITK-SNAP color table. " << std::endl;
}

int main (int argc, char* argv[])
{

  std::cout << std::endl;
  std::cout << "=================================" << std::endl;
  std::cout << "   VRML Converter v1.0.4" << std::endl;
  std::cout << "=================================" << std::endl << std::endl;

  bool doSmoothing = false;
  std::string fnlabel;

  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-s") == 0)
    {
      doSmoothing = true;
    }
    else if (strcmp(argv[i], "-h") == 0)
    {
      usage(std::cout);
      return EXIT_SUCCESS;
    }
    else if (strcmp(argv[i], "-l") == 0)
    {
      ++i;
      if (i == argc)
      {
        usage(std::cerr);
        return EXIT_FAILURE;
      }

      fnlabel = argv[i];
    }
  }

  const char *fninput = argv[argc - 2];
  const char *fnoutput = argv[argc - 1];
  
  
  std::cout << "-- Input Filename: " << fninput << std::endl;
  std::cout << "-- Output Filename: " << fnoutput << std::endl;
  if (doSmoothing)
    std::cout << "-- Smoothing is ON" << std::endl;

  // Build a color table for color lookup
  ColorTable *colorTable = nullptr;
  if (fnlabel.size() > 0)
  {
    std::cout << "-- Using label description file for coloring: " << fnlabel << std::endl;
    colorTable = new ColorTable(fnlabel.c_str());
  }
  else
  {
    std::cout << "-- Using default ITK-SNAP color table for coloring" << std::endl;
    colorTable = new ColorTable();
  }
    

  // Read the image
  typedef uint16_t PixelType;
  typedef itk::Image<PixelType, 3> ImageType;
  typedef itk::Image<double, 3> DoubleImageType;
  typedef itk::ImageFileReader<ImageType> ImageReaderType;
  typedef itk::VTKImageExport<DoubleImageType> VTKExportType;

  ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(fninput);
  reader->Update();
  ImageType::Pointer imageTail = reader->GetOutput();

  // Get the range
  typedef itk::MinimumMaximumImageCalculator<ImageType> MinMaxFilter;
  MinMaxFilter::Pointer fltMinMax = MinMaxFilter::New();
  fltMinMax->SetImage(imageTail);
  fltMinMax->Compute();

  PixelType min = fltMinMax->GetMinimum();
  PixelType max = fltMinMax->GetMaximum();

  //std::cout << "Image Range: min=" << min << "; max=" << max << std::endl;

  // Processing each label
  typedef itk::BinaryThresholdImageFilter<ImageType, DoubleImageType> ThresholdFilter;

  vtkNew<vtkRenderer> renderer;
  uint8_t r, g, b;
  float a;

  for (PixelType i = 1; i <= max; ++i)
    {
    // Threshold the image into binary image
    ThresholdFilter::Pointer fltThreshold = ThresholdFilter::New();
    fltThreshold->SetLowerThreshold(i);
    fltThreshold->SetUpperThreshold(i);
    fltThreshold->SetInsideValue(1);
    fltThreshold->SetOutsideValue(0);
    fltThreshold->SetInput(imageTail);
    fltThreshold->Update();

    // Export ITK to VTK
    VTKExportType::Pointer fltVTKExport = VTKExportType::New();
    fltVTKExport->SetInput(fltThreshold->GetOutput());
    vtkNew<vtkImageImport> fltVTKImport;
    ConnectITKExporterToVTKImporter<DoubleImageType>(fltVTKExport, fltVTKImport);
    fltVTKImport->SetCallbackUserData(fltVTKExport->GetCallbackUserData());
    fltVTKImport->Update();
    vtkSmartPointer<vtkImageData> vtkImageTail = fltVTKImport->GetOutput();

    // Smooth
    if (doSmoothing)
      {
      vtkNew<vtkImageGaussianSmooth> fltSmooth;
      fltSmooth->SetInputData(vtkImageTail);
      fltSmooth->SetStandardDeviation(0.8, 0.8, 0.8);
      fltSmooth->SetRadiusFactors(1.5, 1.5, 1.5);
      fltSmooth->Update();
      vtkImageTail = fltSmooth->GetOutput();
      }


    // Extract Polydata
    vtkNew<vtkMarchingCubes> fltMC;
    fltMC->SetInputData(vtkImageTail);
    fltMC->SetNumberOfContours(1);
    fltMC->SetValue(0, 0.5);
    fltMC->SetComputeGradients(false);
    fltMC->SetComputeNormals(true);
    fltMC->SetComputeScalars(false);
    fltMC->Update();

    // configure actor and mapper
    vtkNew<vtkPolyDataMapper> mapper;
    vtkNew<vtkActor> actor;
    mapper->SetInputData(fltMC->GetOutput());
    actor->SetMapper(mapper);

    colorTable->GetLabelColor(i, r, g, b, a);
    double dr = r/255.0, dg = g/255.0, db = b/255.0;
    //std::cout << "label=" << i << "; color=[" << + dr << ',' << +dg << ',' << +db << "]" << std::endl;
    vtkSmartPointer<vtkProperty> prop = actor->GetProperty();
    prop->SetColor(dr, dg, db);

    renderer->AddActor(actor);
    }

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  /*
  // Debugging: Check the rendering result
  vtkNew<vtkRenderWindowInteractor> interactor;

  vtkNew<vtkInteractorStyleSwitch> styleSwitch;
  styleSwitch->SetCurrentStyleToTrackballCamera();
  interactor->SetInteractorStyle(styleSwitch);
  interactor->SetRenderWindow(renderWindow);
  renderer->ResetCamera();
  renderWindow->SetSize(800, 600);
  renderWindow->Render();
  interactor->Start();
  */

  vtkNew<vtkVRMLExporter> exporter;
  exporter->SetActiveRenderer(renderer);
  exporter->SetRenderWindow(renderWindow);
  exporter->SetFileName(fnoutput);
  exporter->Write();

  std::cout << "-- VRML file exported" << std::endl << std::endl;

  return EXIT_SUCCESS;
}


