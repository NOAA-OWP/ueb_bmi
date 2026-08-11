#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <bmi.hxx>

#include "ControlFile.hxx"
#include "Watershed.hxx"
#include "Parameters.hxx"
#include "SiteVariables.hxx"
#include "ForcingVariables.hxx"
#include "OutControl.hxx"
#include "bmi_ueb.hxx"
#include "Logger.hpp"

std::stringstream bmimain_ss("");


int main(int argc, char *argv[])
{

  if (argc != 2) {
    printf("Usage: run_bmiuebcxx CONTROL_FILE\n\n");
    printf("Run the uebcxx model through its BMI with a control file.\n");
    return bmi::BMI_SUCCESS;
  }

  int year, month, day, hour;

  //create the model object
  ueb::BmiUEB* model = bmi_model_create();

  bmimain_ss << "Model name: " << model->GetComponentName() << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");

  //setup the model
  bmimain_ss << "Setup the model." << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
  model->Initialize(argv[1]);

  void* ts = new float();

  model->GetValue( std::string( "ts" ), ts );
  bmimain_ss << "ts = " << *(float*)ts << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
/*
  float newts = 2.f;
  model->SetValue( std::string( "ts" ), &newts );
  model->GetValue( std::string( "ts" ), ts );
  bmimain_ss << "New ts = " << *(float*)ts << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
*/

  //get the start time
  model->GetStartTimeInGregorianCalendar( year, month, day, hour );

  bmimain_ss << "Model start datetime is " << year << '/' << month << '/'
	                                  << day << ' ' << hour << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
  //get the end time
  model->GetEndTimeInGregorianCalendar( year, month, day, hour );

  bmimain_ss << "Model end datetime is " << year << '/' << month << '/'
	                                  << day << ' ' << hour << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
  //advance the model one timestep and pause
  
  bmimain_ss << "Advance the model one timestep and pause." << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
  model->Update();

  //get the current time
  model->GetCurrentTimeInGregorianCalendar( year, month, day, hour );

  bmimain_ss << "Model current datetime is " << year << '/' << month << '/'
	                                  << day << ' ' << hour << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");

  double currenttime = model->GetCurrentTime();

  double tendaysafter = currenttime + 10.0 * 24 * 3600;

  //advance the model 10 days and  pause
  //
  bmimain_ss << "Advance the model ten days and pause." << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
  model->UpdateUntil( tendaysafter );
  model->GetCurrentTimeInGregorianCalendar( year, month, day, hour );
  bmimain_ss << "Model current datetime is " << year << '/' << month << '/'
	                                  << day << ' ' << hour << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");

  double endtime = model->GetEndTime();

  //advance to the end
  bmimain_ss << "Advance the model to the end." << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
  model->UpdateUntil( endtime );

  model->GetCurrentTimeInGregorianCalendar( year, month, day, hour );
  bmimain_ss << "Model current datetime is " << year << '/' << month << '/'
	                                  << day << ' ' << hour << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
  //cleaning up
  bmimain_ss << "Output and clean up." << std::endl;
  LOG(bmimain_ss.str(), LogLevel::INFO); bmimain_ss.str("");
  model->Finalize();

  //destroy the mdoel
  bmi_model_destroy( model );

  return bmi::BMI_SUCCESS;
}
