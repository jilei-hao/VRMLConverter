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

int main (int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: VRMLConverter input.vtk output.vrml" << std::endl;
        return EXIT_FAILURE;
    }

    const char *fninput = argv[1];
    const char *fnoutput = argv[2];
    std::cout << "-- Input Filename: " << fninput << std::endl;
    std::cout << "-- Output Filename: " << fnoutput << std::endl;

    vtkNew<vtkPolyDataReader> reader;
    reader->SetFileName(fninput);
    reader->Update();
    vtkSmartPointer<vtkPolyData> polydata = reader->GetOutput();
    
    vtkNew<vtkActor> actor;
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(polydata);
    mapper->SetColorModeToMapScalars();
    mapper->SetScalarModeToUsePointData();
    actor->SetMapper(mapper);

    auto scalars = polydata->GetPointData()->GetScalars();
    double *range;
    range = scalars->GetRange();

    vtkNew<vtkLookupTable> lut;
    lut->SetTableRange(range);
    lut->Build();
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(range[0], range[1]);
    
    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkRenderWindow> renderWindow;
    
    vtkNew<vtkProperty> property;
    property->SetColor(255,0,0);
    actor->SetProperty(property);
    renderer->AddActor(actor);
    renderWindow->AddRenderer(renderer);

    /*
    // Debugging: Check the rendering result
    vtkNew<vtkRenderWindowInteractor> interactor;
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