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

// The Color list from SNAP
// https://github.com/pyushkevich/itksnap/blob/312323ea9477ae8ee2c47dd20a93b621527d71d8/Logic/Common/ColorLabelTable.cxx#L39

const size_t ColorListSize = 130;
const char *ColorList[ColorListSize] = {
  "#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#00FFFF", "#FF00FF",
  "#FFEFD5", "#0000CD", "#CD853F", "#D2B48C", "#66CDAA", "#000080",
  "#008B8B", "#2E8B57", "#FFE4E1", "#6A5ACD", "#DDA0DD", "#E9967A",
  "#A52A2A", "#FFFAFA", "#9370DB", "#DA70D6", "#4B0082", "#FFB6C1",
  "#3CB371", "#FFEBCD", "#FFE4C4", "#DAA520", "#008080", "#BC8F8F",
  "#FF69B4", "#FFDAB9", "#DEB887", "#7FFF00", "#8B4513", "#7CFC00",
  "#FFFFE0", "#4682B4", "#006400", "#EE82EE", "#EEE8AA", "#F0FFF0",
  "#F5DEB3", "#B8860B", "#20B2AA", "#FF1493", "#191970", "#708090",
  "#228B22", "#F8F8FF", "#F5FFFA", "#FFA07A", "#90EE90", "#ADFF2F",
  "#4169E1", "#FF6347", "#FAF0E6", "#800000", "#32CD32", "#F4A460",
  "#FFFFF0", "#7B68EE", "#FFA500", "#ADD8E6", "#FFC0CB", "#7FFFD4",
  "#FF8C00", "#8FBC8F", "#DC143C", "#FDF5E6", "#FFFAF0", "#00CED1",
  "#00FF7F", "#800080", "#FFFACD", "#FA8072", "#9400D3", "#B22222",
  "#FF7F50", "#87CEEB", "#6495ED", "#F0E68C", "#FAEBD7", "#FFF5EE",
  "#6B8E23", "#87CEFA", "#00008B", "#8B008B", "#F5F5DC", "#BA55D3",
  "#FFE4B5", "#FFDEAD", "#00BFFF", "#D2691E", "#FFF8DC", "#2F4F4F",
  "#483D8B", "#AFEEEE", "#808000", "#B0E0E6", "#FFF0F5", "#8B0000",
  "#F0FFFF", "#FFD700", "#D8BFD8", "#778899", "#DB7093", "#48D1CC",
  "#FF00FF", "#C71585", "#9ACD32", "#BDB76B", "#F0F8FF", "#E6E6FA",
  "#00FA9A", "#556B2F", "#40E0D0", "#9932CC", "#CD5C5C", "#FAFAD2",
  "#5F9EA0", "#008000", "#FF4500", "#E0FFFF", "#B0C4DE", "#8A2BE2",
  "#1E90FF", "#F08080", "#98FB98", "#A0522D"};

int parse_color(
  const char* p, unsigned char& r, unsigned char& g, unsigned char& b)
{
  // Must be a seven-character string
  if(strlen(p) != 7)
    return EXIT_FAILURE;

  int val[6];
  for(int i = 0; i < 6; i++)
    {
    char c = p[i+1];
    if(c >= 'A' && c <= 'F')
      val[i] = 10 + (c - 'A');
    else if(c >= 'a' && c <= 'f')
      val[i] = 10 + (c - 'a');
    else if(c >= '0' && c <= '9')
      val[i] = c - '0';
    else return -1;
    }

  r = 16 * val[0] + val[1];
  g = 16 * val[2] + val[3];
  b = 16 * val[4] + val[5];
  return EXIT_SUCCESS;
}

void GetLabelColor(unsigned int label, uint8_t &r, uint8_t &g, uint8_t &b)
{
  parse_color(ColorList[(label-1) % ColorListSize], r, g, b);
}


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

int main (int argc, char* argv[])
{
  if (argc != 3)
  {
    std::cerr << "Usage: VRMLConverter input.nii.gz output.vrml" << std::endl;
    return EXIT_FAILURE;
  }

  const char *fninput = argv[1];
  const char *fnoutput = argv[2];
  std::cout << "-- Input Filename: " << fninput << std::endl;
  std::cout << "-- Output Filename: " << fnoutput << std::endl;

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

  //sstd::cout << "Image Range: min=" << min << "; max=" << max << std::endl;

  // Processing each label
  typedef itk::BinaryThresholdImageFilter<ImageType, DoubleImageType> ThresholdFilter;

  vtkNew<vtkRenderer> renderer;
  uint8_t r, g, b;

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
    vtkNew<vtkImageGaussianSmooth> fltSmooth;
    fltSmooth->SetInputData(vtkImageTail);
    fltSmooth->SetStandardDeviation(0.8, 0.8, 0.8);
    fltSmooth->SetRadiusFactors(1.5, 1.5, 1.5);
    fltSmooth->Update();

    // Extract Polydata
    vtkNew<vtkMarchingCubes> fltMC;
    fltMC->SetInputData(fltSmooth->GetOutput());
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

    GetLabelColor(i, r, g, b);
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

  std::cout << "-- VRML file exported" << std::endl;

  return EXIT_SUCCESS;
}


