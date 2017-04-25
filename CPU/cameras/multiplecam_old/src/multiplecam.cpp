//============================================================================
// Name        : multiplecam.cpp
// Author      : Sara Turriziani
// Version     : 3.1
// Copyright   : Mini-EUSO copyright notice
// Description : Cameras Acquisition Module in C++, ANSI-style, for linux
//============================================================================

#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sstream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "/usr/include/flycapture/FlyCapture2.h"
using namespace FlyCapture2;
using namespace std;

unsigned createMask(unsigned a, unsigned b)
{
	unsigned int r = 0;
	for (unsigned i = a; i < b; i++)
	{
		r = r+1;
		r  = r*2;
	}
	r = r + 1;
	return r;
}

// test time in ms
const std::string currentDateTime2() {
 timeval curTime;
 gettimeofday(&curTime, NULL);
 int milli = curTime.tv_usec / 1000;
 struct tm timeinfo;
 char buffer [80];
 strftime(buffer, sizeof(buffer), "%Y-%m-%d.%H-%M-%S", localtime_r(&curTime.tv_sec, &timeinfo));
 char currentTime[84] = "";
 sprintf(currentTime, "%s.%03d", buffer, milli);

 return currentTime;

  }

void PrintBuildInfo()
{
    FC2Version fc2Version;
    Utilities::GetLibraryVersion( &fc2Version );
    char version[128];
    sprintf(
        version,
        "FlyCapture2 library version: %d.%d.%d.%d\n",
        fc2Version.major, fc2Version.minor, fc2Version.type, fc2Version.build );

    printf( version );

    char timeStamp[512];
    sprintf( timeStamp, "Application build date: %s %s\n\n", __DATE__, __TIME__ );

    printf( timeStamp );
}

void PrintCameraInfo( CameraInfo* pCamInfo )
{
    printf(
        "\n*** CAMERA INFORMATION ***\n"
        "Serial number - %u\n"
        "Camera model - %s\n"
        "Camera vendor - %s\n"
        "Sensor - %s\n"
        "Resolution - %s\n"
        "Firmware version - %s\n"
        "Firmware build time - %s\n\n",
        pCamInfo->serialNumber,
        pCamInfo->modelName,
        pCamInfo->vendorName,
        pCamInfo->sensorInfo,
        pCamInfo->sensorResolution,
        pCamInfo->firmwareVersion,
        pCamInfo->firmwareBuildTime );
}

void PrintError( Error error )
{
    error.PrintErrorTrace();
}

enum ExtendedShutterType
{
    NO_EXTENDED_SHUTTER,
    GENERAL_EXTENDED_SHUTTER
};

//int main(int /*argc*/, char** /*argv*/)
int main(int argc, char* argv[])
{

	unsigned int ulValue;
	CameraInfo camInfo;

    // Check the number of parameters
    if (argc < 2) {
        // Tell the user how to run the program if the user enters the command incorrectly.
        std::cerr << "Usage: " << argv[0] << " PATH" << std::endl; // Usage message
        return 1;
    }

//    cout << "Input: " << argv[1] << endl; // uncomment for debugging

    std:string pardir = argv[1];

    PrintBuildInfo();
    Error error;
    // Since this application saves images in the current folder
    // we must ensure that we have permission to write to this folder.
    // If we do not have permission, fail right away.
	FILE* tempFile = fopen("test.txt", "w+");
	if (tempFile == NULL)
	{
		printf("Failed to create file in current folder.  Please check permissions.\n");
		return -1;
	}
	fclose(tempFile);
	remove("test.txt");

    BusManager busMgr;
    unsigned int numCameras;
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK)
    {
        PrintError( error );
        return -1;
    }

    printf( "Number of cameras detected: %u\n", numCameras );

    Camera *pCameras = new Camera[numCameras]; // initialize an array of cameras

    for (unsigned int i=0; i < numCameras; i++)
          {
            PGRGuid guid;
            error = busMgr.GetCameraFromIndex(i, &guid);
            if (error != PGRERROR_OK)
             {
                PrintError( error );
                delete[] pCameras;
                return -1;
             }
            error = pCameras[i].Connect(&guid); // connect both cameras
            if (error != PGRERROR_OK)
              {
                PrintError(error);
                delete[] pCameras;
                return -1;
               }

          error = pCameras[i].GetCameraInfo(&camInfo);
          if (error != PGRERROR_OK)
           {
             PrintError(error);
             delete[] pCameras;
             return -1;
            }

          PrintCameraInfo(&camInfo);


          // Turn Timestamp on

          EmbeddedImageInfo imageInfo;
          imageInfo.timestamp.onOff = true;
          error = pCameras[i].SetEmbeddedImageInfo(&imageInfo);
          if (error != PGRERROR_OK)
           {
             PrintError(error);
             delete[] pCameras;
             return -1;
           }

          // read from the camera register the default factory value for each parameter

          Property frmRate;
          frmRate.type = FRAME_RATE;
          error = pCameras[i].GetProperty( &frmRate );
          if (error != PGRERROR_OK)
            {
              PrintError( error );
              delete[] pCameras;
              return -1;
            }

          PropertyInfo frRate;
          frRate.type = FRAME_RATE;
          error = pCameras[i].GetPropertyInfo( &frRate );
          if (error != PGRERROR_OK)
            {
              PrintError( error );
              delete[] pCameras;
              return -1;
            }
          float minfrmRate = frRate.absMin;
          float maxfrmRate = frRate.absMax;

          printf( "Default FRAME_RATE is %3.1f fps. \n" , frmRate.absValue );

        //  printf("Test name %s\n", camInfo.modelName);

             char* name1;
             name1 = strtok(camInfo.modelName, " ");

           //  cout << name1 << '\n'; uncomment for debugging

           if ( strcmp("Chameleon", name1) == 0)
            {
        	   cout << "This is NIR camera" << endl;
           }

          if ( strcmp("Chameleon", name1) == 0) // read the temperature register only for NIR camera
            {
              error = pCameras[i].ReadRegister(0x82C, &ulValue); // read the temperature register
              if (error != PGRERROR_OK)
                {
                  PrintError( error );
                  delete[] pCameras;
                  return -1;
                }

              unsigned r = createMask(20, 31); // extract the bits of interest
              unsigned int res = r & ulValue;  // here you have already the Kelvin temperature * 10
              cout << "Initial Temperature is " << res / 10.0 << " K - " << res / 10.0	- 273.15 << " Celsius" << endl;
          }

          //Update the register of each camera reading from parfile

          std::string line;
          std::string parfilename;



          float frate, shutt, gain, autoexpo;
          char frset[3], brset[3], autexpset[3];
          int desiredBrightness;

          ifstream parfile;
          string destination;

          if ( strcmp("Chameleon", name1) == 0)
            {
        	  std::stringstream pf;
        	  pf << pardir  << "/testNIR.ini";
        	  parfilename = pf.str();
        	  parfile.open(parfilename.c_str());
        	 destination = "NIR";
            }
          else
          {
        	  std::stringstream pf;
        	  pf << pardir  << "/testVIS.ini";
        	  parfilename = pf.str();
        	 parfile.open(parfilename.c_str());
        	  destination = "VIS";
          }


          // Set the new values from the parameters reading them from the parameter file
          if (parfile.is_open())
           {
              printf( "Reading from parfile: %s \n", parfilename.c_str() );
              while ( getline (parfile,line) )
                   {
                     std::istringstream in(line);      //make a stream for the line itself
                     std::string type;
                     in >> type;                  //and read the first whitespace-separated token


                     if(type == "FRAMERATE")       //and check its value
                      {
                        in >> frate;       //now read the whitespace-separated floats
                        cout << type << " " << frate << " fps " << endl;
                      }
                     else if(type == "SHUTTER")
                      {
                        in >> shutt;
                        cout << type << " " << shutt << " ms " << endl;
                      }
                     else if((type == "GAIN"))
                      {
                        in >> gain;
                        cout << type << " " << gain << endl;
                      }
                     else if((type == "BRIGHTNESS"))
                      {
                       in >> desiredBrightness;
                       cout << type << " " << desiredBrightness << endl;
                      }
                     else if((type == "AUTO_EXPOSURE"))
                      {
                        in >> autoexpo;
                        cout << type << " " << autoexpo << endl;
                      }

                     if(type == "FRAMERATESET")
                      {
                        in >> frset;
                  //    cout << "Setting the frame rate " << frset << endl; // uncomment for debugging purposes
                      }
                     if(type == "BRIGHTSET")
                      {
                       in >> brset;
                    //            cout << "Setting the brightness " << frset << endl; // uncomment for debugging purposes
                      }
                     if(type == "AUTOEXPOSET")
                       {
                          in >> autexpset;
                      //            cout << "Setting the autoexposure " << autexpset << endl; // uncomment for debugging purposes
                       }

                }
            parfile.close();

           }
            else
                {
                 printf( "Unable to open parfile!!! \n" );
                 return -1;
                }




          // Turn off SHARPNESS if the camera supports this property
           PropertyInfo propInfo;
           propInfo.type = SHARPNESS;
           error =  pCameras[i].GetPropertyInfo( &propInfo );
           if (error != PGRERROR_OK)
             {
                PrintError( error );
                delete[] pCameras;
                return -1;
             }

           if ( propInfo.present == true )
             {
              // Then turn off SHARPNESS
                Property prop;
                prop.type = SHARPNESS;
                prop.autoManualMode = false;
                prop.onOff = false;

                error = pCameras[i].SetProperty( &prop );
                if (error != PGRERROR_OK)
                       {
                         PrintError( error );
                         delete[] pCameras;
                         return -1;
                       }
                cout << "Setting off SHARPNESS" << endl;
             }
           else
             {
               printf( "This camera does not support SHARPNESS \n");
             }

           // Turn off SATURATION if the camera supports this property

                      propInfo.type = SATURATION;
                      error =  pCameras[i].GetPropertyInfo( &propInfo );
                      if (error != PGRERROR_OK)
                        {
                           PrintError( error );
                           delete[] pCameras;
                           return -1;
                        }

                      if ( propInfo.present == true )
                        {
                         // Then turn off SATURATION
                           Property prop;
                           prop.type = SATURATION;
                           prop.autoManualMode = false;
                           prop.onOff = false;

                           error = pCameras[i].SetProperty( &prop );
                           if (error != PGRERROR_OK)
                                  {
                                    PrintError( error );
                                    delete[] pCameras;
                                    return -1;
                                  }
                           cout << "Setting off SATURATION" << endl;
                        }
                      else
                        {
                          printf( "This camera does not support SATURATION \n");
                        }

                      // Turn off IRIS if the camera supports this property

                                 propInfo.type = IRIS;
                                 error =  pCameras[i].GetPropertyInfo( &propInfo );
                                 if (error != PGRERROR_OK)
                                   {
                                      PrintError( error );
                                      delete[] pCameras;
                                      return -1;
                                   }

                                 if ( propInfo.present == true )
                                   {
                                    // Then turn off IRIS
                                      Property prop;
                                      prop.type = IRIS;
                                      prop.autoManualMode = false;
                                      prop.onOff = false;

                                      error = pCameras[i].SetProperty( &prop );
                                      if (error != PGRERROR_OK)
                                             {
                                               PrintError( error );
                                               delete[] pCameras;
                                               return -1;
                                             }
                                      cout << "Setting off IRIS" << endl;
                                   }
                                 else
                                   {
                                     printf( "This camera does not support IRIS \n");
                                   }


           // Turn off WHITE_BALANCE if the camera supports this property

            propInfo.type = WHITE_BALANCE;
            
             error =  pCameras[i].GetPropertyInfo( &propInfo );
                    if (error != PGRERROR_OK)
                      {
                        PrintError( error );
                        delete[] pCameras;
                        return -1;
                      }

                    if ( propInfo.present == true )
                     {
                      // Then turn off WHITE_BALANCE
                      Property prop;
                      prop.type = WHITE_BALANCE;
                      prop.autoManualMode = false;
                      prop.onOff = false;

                      error = pCameras[i].SetProperty( &prop );
                      if (error != PGRERROR_OK)
                        {
                          PrintError( error );
                          delete[] pCameras;
                          return -1;
                        }
                      cout << "Setting off WHITE_BALANCE" << endl;
                     }
                    else
                       {
                         printf( "This camera does not support WHITE_BALANCE \n");
                       }

                    // Turn off HUE if the camera supports this property

                     propInfo.type = HUE;
                     error =  pCameras[i].GetPropertyInfo( &propInfo );
                     if (error != PGRERROR_OK)
                       {
                         PrintError( error );
                         delete[] pCameras;
                         return -1;
                       }

                     if ( propInfo.present == true )
                       {
                         // Then turn off HUE
                        Property prop;
                        prop.type = HUE;
                        prop.autoManualMode = false;
                        prop.onOff = false;

                        error = pCameras[i].SetProperty( &prop );
                        if (error != PGRERROR_OK)
                          {
                            PrintError( error );
                            delete[] pCameras;
                            return -1;
                           }
                        cout << "Setting off HUE" << endl;
                        }
                     else
                     {
                    	 printf( "This camera does not support HUE \n");
                     }

                     // Turn off GAMMA if the camera supports this property

                      propInfo.type = GAMMA;
                      error =  pCameras[i].GetPropertyInfo( &propInfo );
                      if (error != PGRERROR_OK)
                        {
                          PrintError( error );
                          delete[] pCameras;
                          return -1;
                        }

                       if ( propInfo.present == true )
                         {
                          // Then turn off GAMMA
                             Property prop;
                             prop.type = GAMMA;
                             prop.autoManualMode = false;
                             prop.onOff = false;

                             error = pCameras[i].SetProperty( &prop );
                             if (error != PGRERROR_OK)
                               {
                                 PrintError( error );
                                 delete[] pCameras;
                                 return -1;
                               }
                                cout << "Setting off GAMMA" << endl;
                               }
                         else
                              {
                                printf( "This camera does not support GAMMA \n");
                              }
                       // Turn off PAN if the camera supports this property

                        propInfo.type = PAN;
                        error =  pCameras[i].GetPropertyInfo( &propInfo );
                        if (error != PGRERROR_OK)
                          {
                                    PrintError( error );
                                    delete[] pCameras;
                                    return -1;
                          }

                     if ( propInfo.present == true )
                       {
                                  // Then turn off PAN
                          Property prop;
                          prop.type = PAN;
                          prop.autoManualMode = false;
                          prop.onOff = false;

                          error = pCameras[i].SetProperty( &prop );
                          if (error != PGRERROR_OK)
                            {
                               PrintError( error );
                               delete[] pCameras;
                               return -1;
                            }
                           cout << "Setting off PAN" << endl;
                        }
                    else
                        {
                          printf( "This camera does not support PAN \n");
                        }

                     // Turn off TILT if the camera supports this property

                      propInfo.type = TILT;
                      error =  pCameras[i].GetPropertyInfo( &propInfo );
                      if (error != PGRERROR_OK)
                        {
                                  PrintError( error );
                                  delete[] pCameras;
                                  return -1;
                        }

                      if ( propInfo.present == true )
                               {
                                // Then turn off TILT
                                Property prop;
                                prop.type = TILT;
                                prop.autoManualMode = false;
                                prop.onOff = false;

                                error = pCameras[i].SetProperty( &prop );
                                if (error != PGRERROR_OK)
                                  {
                                    PrintError( error );
                                    delete[] pCameras;
                                    return -1;
                                  }
                                cout << "Setting off TILT" << endl;
                               }
                      else
                                   {
                                    printf( "This camera does not support TILT \n");
                                   }

                      // Turn off ZOOM if the camera supports this property

                       propInfo.type = ZOOM;
                               error =  pCameras[i].GetPropertyInfo( &propInfo );
                               if (error != PGRERROR_OK)
                                 {
                                   PrintError( error );
                                   delete[] pCameras;
                                   return -1;
                                 }

                               if ( propInfo.present == true )
                                {
                                 // Then turn off ZOOM
                                 Property prop;
                                 prop.type = ZOOM;
                                 prop.autoManualMode = false;
                                 prop.onOff = false;

                                 error = pCameras[i].SetProperty( &prop );
                                 if (error != PGRERROR_OK)
                                   {
                                     PrintError( error );
                                     delete[] pCameras;
                                     return -1;
                                   }
                                 cout << "Setting off ZOOM" << endl;
                                }
                                else
                                    {
                                     printf( "This camera does not support ZOOM \n");
                                    }

                               // Turn off TRIGGER_MODE if the camera supports this property

                                propInfo.type = TRIGGER_MODE;
                                        error =  pCameras[i].GetPropertyInfo( &propInfo );
                                        if (error != PGRERROR_OK)
                                          {
                                            PrintError( error );
                                            delete[] pCameras;
                                            return -1;
                                          }

                                        if ( propInfo.present == true )
                                         {
                                          // Then turn off TRIGGER_MODE
                                          Property prop;
                                          prop.type = TRIGGER_MODE;
                                          prop.autoManualMode = false;
                                          prop.onOff = false;

                                          error = pCameras[i].SetProperty( &prop );
                                          if (error != PGRERROR_OK)
                                            {
                                              PrintError( error );
                                              delete[] pCameras;
                                              return -1;
                                            }
                                          cout << "Setting off TRIGGER_MODE" << endl;
                                         }
                                         else
                                             {
                                              printf( "This camera does not support TRIGGER_MODE \n");
                                             }

                                        // Turn off TRIGGER_DELAY if the camera supports this property

                                         propInfo.type = TRIGGER_DELAY;
                                                 error =  pCameras[i].GetPropertyInfo( &propInfo );
                                                 if (error != PGRERROR_OK)
                                                   {
                                                     PrintError( error );
                                                     delete[] pCameras;
                                                     return -1;
                                                   }

                                                 if ( propInfo.present == true )
                                                  {
                                                   // Then turn off TRIGGER_DELAY
                                                   Property prop;
                                                   prop.type = TRIGGER_DELAY;
                                                   prop.autoManualMode = false;
                                                   prop.onOff = false;

                                                   error = pCameras[i].SetProperty( &prop );
                                                   if (error != PGRERROR_OK)
                                                     {
                                                       PrintError( error );
                                                       delete[] pCameras;
                                                       return -1;
                                                     }
                                                   cout << "Setting off TRIGGER_DELAY" << endl;
                                                  }
                                                  else
                                                      {
                                                       printf( "This camera does not support TRIGGER_DELAY \n");
                                                      }


         //  check if the camera supports the FRAME_RATE property

             propInfo.type = FRAME_RATE;
             error =  pCameras[i].GetPropertyInfo( &propInfo );
             if (error != PGRERROR_OK)
               {
                 PrintError( error );
                 delete[] pCameras;
                 return -1;
                }

          if ( propInfo.present == true )
           {
              if ( strcmp("OFF",frset) == 0 || strcmp("OFf",frset)  == 0|| strcmp("OfF",frset)  == 0 || strcmp("Off",frset)  == 0 || strcmp("oFF",frset)  == 0  || strcmp("oFf",frset) == 0 || strcmp("ofF",frset) == 0 || strcmp("off",frset) == 0)
                 {
                  cout << "Setting " << frset << " FRAME_RATE "  << endl;

                  ExtendedShutterType shutterType = NO_EXTENDED_SHUTTER;
                // Then turn off frame rate
                Property prop;
                prop.type = FRAME_RATE;
                error =  pCameras[i].GetProperty( &prop );
                if (error != PGRERROR_OK)
                  {
                    PrintError( error );
                    delete[] pCameras;
                    return -1;
                  }

                prop.autoManualMode = false;
                prop.onOff = false;
                error =  pCameras[i].SetProperty( &prop );
                if (error != PGRERROR_OK)
                  {
                    PrintError( error );
                    delete[] pCameras;
                    return -1;
                  }
               shutterType = GENERAL_EXTENDED_SHUTTER;
            }
              else
                  {
                    PropertyInfo frRate;
                         	frRate.type = FRAME_RATE;
                         	error = pCameras[i].GetPropertyInfo( &frRate );
                         	if (error != PGRERROR_OK)
                                     {
                         	          PrintError( error );
                         	          delete[] pCameras;
                         	          return -1;
                         	         }
                         	 float minfrmRate = frRate.absMin;
                         	 float maxfrmRate = frRate.absMax;

                         	 Property frmRate;
                         	 frmRate.type = FRAME_RATE;
                         	 frmRate.absControl = true;
                         	 frmRate.onePush = false;
                         	 frmRate.autoManualMode = false;
                         	 frmRate.onOff = true;

                         	 if (frate >= minfrmRate && frate <= maxfrmRate)
                         	   {
                         		 printf( "Frame rate set to %3.2f fps\n", frate );
                         	     frmRate.absValue = frate;
                         	   }

                         	 else{
                         	        	 printf( "Frame Rate outside allowed range. Abort. \n" );
                         	        	 delete[] pCameras;
                         	        	 return -1;
                         	      }

                         	 error = pCameras[i].SetProperty( &frmRate );
                         	 if (error != PGRERROR_OK)
                         	        {
                         	          PrintError( error );
                         	          delete[] pCameras;
                         	          return -1;
                         	        }
                               }
                  }
            else
             {
               printf( "Frame rate and extended shutter are not supported... exiting\n" );
               delete[] pCameras;
               return -1;
             }




            // Set the shutter property of the camera

            PropertyInfo Shut;
                    Shut.type = SHUTTER;
                    error = pCameras[i].GetPropertyInfo( &Shut );
                    if (error != PGRERROR_OK)
                    {
                       PrintError( error );
                       return -1;
                    }
                    float minShutter = Shut.absMin;
                    float maxShutter = Shut.absMax;

                    // printf( "Min %3.1f ms Max %3.1f ms  \n" , Shut.absMin, Shut.absMax ); // uncomment this line to debug

             Property shutter;
             shutter.type = SHUTTER;
             shutter.absControl = true;
             shutter.onePush = false;
             shutter.autoManualMode = false;
             shutter.onOff = true;
             shutter.absValue = shutt;

             if (shutt >= minShutter && shutt <= maxShutter)
               {
                 shutter.absValue = shutt;
                 printf( "Shutter time set to %3.2f ms\n", shutt );
               }
            else{
                  printf( "WARNING! Shutter outside allowed range: setting it to the maximum allowed value %3.2f ms \n", maxShutter );
                  shutter.absValue = maxShutter;
                }

           error = pCameras[i].SetProperty( &shutter );
           if (error != PGRERROR_OK)
             {
               PrintError( error );
               delete[] pCameras;
               return -1;
             }


           //  check if the camera supports the AUTO_EXPOSURE property

            propInfo.type = AUTO_EXPOSURE;
            error =  pCameras[i].GetPropertyInfo( &propInfo );
            if (error != PGRERROR_OK)
              {
                 PrintError( error );
                 delete[] pCameras;
                 return -1;
              }

           if ( propInfo.present == true )
            {
               if ( strcmp("OFF",autexpset) == 0 || strcmp("OFf",autexpset)  == 0|| strcmp("OfF",autexpset)  == 0 || strcmp("Off",autexpset)  == 0 || strcmp("oFF",autexpset)  == 0  || strcmp("oFf",autexpset) == 0 || strcmp("ofF",autexpset) == 0 || strcmp("off",autexpset) == 0)
                {
                   cout << "Setting " << autexpset << " AUTO_EXPOSURE "  << endl;

                  // Then turn off AUTO_EXPOSURE
                     Property prop;
                     prop.type = AUTO_EXPOSURE;
                     error =  pCameras[i].GetProperty( &prop );
                     if (error != PGRERROR_OK)
                       {
                         PrintError( error );
                         delete[] pCameras;
                         return -1;
                       }

                                            prop.autoManualMode = false;
                                            prop.onOff = false;
                                            error =  pCameras[i].SetProperty( &prop );
                                            if (error != PGRERROR_OK)
                                              {
                                                PrintError( error );
                                                delete[] pCameras;
                                                return -1;
                                              }

                                             }
                                          else
                                              {
                                                PropertyInfo propinfo;
                                                propinfo.type = AUTO_EXPOSURE;
                                                error = pCameras[i].GetPropertyInfo( &propinfo );
                                                if (error != PGRERROR_OK)
                                                  {
                                                    PrintError( error );
                                                    delete[] pCameras;
                                                    return -1;
                                                   }

                                                   Property prop;
                                                   prop.absControl = true;
                                                   prop.type = AUTO_EXPOSURE;
                                                   prop.onePush = false;
                                                   prop.autoManualMode = false;
                                                   prop.onOff = true;

                                                if (autoexpo >= propInfo.absMin && autoexpo <= propInfo.absMax)
                                                 {
                                          	           prop.absValue = autoexpo;
                                                      error = pCameras[i].SetProperty( &prop );
                                                      if (error != PGRERROR_OK)
                                                         {
                                                             PrintError( error );
                                                             delete[] pCameras;
                                                             return -1;
                                                         }
                                                      printf( "AUTO_EXPOSURE set to %3.3f  \n", autoexpo );
                                                 }
                                               else
                                                 {
                                                  printf( "WARNING! AUTO_EXPOSURE outside allowed range [%3.2f, %3.2f] leaving it to default value \n", propInfo.absMin, propInfo.absMax);
                                                }
                                        }
                                      }



           //Set gain checking if the property is supported


             propInfo.type = GAIN;
             error =  pCameras[i].GetPropertyInfo( &propInfo );
             if (error != PGRERROR_OK)
               {
                 PrintError( error );
                 delete[] pCameras;
                 return -1;
               }
             if ( propInfo.present == true )
               {
                  Property prop;
                  prop.type = GAIN;
                  prop.absControl = true;
                  prop.onePush = false;
                  prop.autoManualMode = false;
                  prop.onOff = true;


                  if (gain >= propInfo.absMin && gain <= propInfo.absMax)
                         {
                           	 prop.absValue = gain;
                             error = pCameras[i].SetProperty( &prop );
                             if (error != PGRERROR_OK)
                                  {
                                    PrintError( error );
                                    delete[] pCameras;
                                    return -1;
                                  }
                             printf( "Gain set to %3.2f dB \n", gain );
                          }
                else
                       {
                          printf( "WARNING! Gain outside allowed range [%3.2f, %3.2f] leaving it to default value \n", propInfo.absMin, propInfo.absMax);
                       }
                   }

             //  check if the camera supports the BRIGHTNESS property

                          propInfo.type = BRIGHTNESS;
                          error =  pCameras[i].GetPropertyInfo( &propInfo );
                          if (error != PGRERROR_OK)
                            {
                              PrintError( error );
                              delete[] pCameras;
                              return -1;
                             }

                       if ( propInfo.present == true )
                        {
                           if ( strcmp("OFF",brset) == 0 || strcmp("OFf",brset)  == 0|| strcmp("OfF",brset)  == 0 || strcmp("Off",brset)  == 0 || strcmp("oFF",brset)  == 0  || strcmp("oFf",brset) == 0 || strcmp("ofF",brset) == 0 || strcmp("off",brset) == 0)
                              {
                               cout << "Setting " << brset << " BRIGHTNESS "  << endl;


                             // Then turn off BRIGHTNESS
                             Property prop;
                             prop.type = BRIGHTNESS;
                             error =  pCameras[i].GetProperty( &prop );
                             if (error != PGRERROR_OK)
                               {
                                 PrintError( error );
                                 delete[] pCameras;
                                 return -1;
                               }

                             prop.autoManualMode = false;
                             prop.onOff = false;
                             error =  pCameras[i].SetProperty( &prop );
                             if (error != PGRERROR_OK)
                               {
                                 PrintError( error );
                                 delete[] pCameras;
                                 return -1;
                               }

                         }
                           else
                               {
                                 PropertyInfo propinfo;
                                 propinfo.type = BRIGHTNESS;
                                 error = pCameras[i].GetPropertyInfo( &propinfo );
                                 if (error != PGRERROR_OK)
                                   {
                                     PrintError( error );
                                     delete[] pCameras;
                                     return -1;
                                    }


             			   int minbr = propinfo.min;
             			   int maxbr = propinfo.max;
             			// cout << "Minimum: " << minbr << "  - maximum:  " << maxbr << endl; // uncomment for debugging

                                    Property prop;
                                    prop.type = BRIGHTNESS;
                                    prop.onePush = false;
                                    prop.autoManualMode = false;
                                    prop.onOff = true;

                                    if (desiredBrightness >= minbr && desiredBrightness <= maxbr)
                                      {
                                        prop.valueA = desiredBrightness;
                                        printf( "BRIGHTNESS set to %4d \n", desiredBrightness );
                                        error = pCameras[i].SetProperty( &prop );
                                        if (error != PGRERROR_OK)
                                          {
                                            PrintError( error );
                                            delete[] pCameras;
                                            return -1;
                                          }
                                      }
                                   else{
                                         printf( "WARNING! BRIGHTNESS outside allowed range [%4d, %4d] leaving it to default value \n", minbr, maxbr );
                                       }
                                 }

                               }
                         else
                          {
                            printf( "Warning: BRIGHTNESS is not supported... \n" );
                          }


            
            // Start streaming on camera
            std::stringstream ss;
            ss << currentDateTime2();
            std::string st = ss.str();
            char pippo[st.length()];
            sprintf(pippo, "%s" , st.c_str() );
            printf( "Start time %s \n", pippo );


            error = pCameras[i].StartCapture();
            if (error != PGRERROR_OK)
              {
                PrintError(error);
                delete[] pCameras;
                return -1;
              }
          }

  //  for ( int imageCnt=0; imageCnt < k_numImages; imageCnt++ )
    int imageCnt=0; // uncomment this and other lines beginning with /// & comment previous line to get an indefinite loop
    for ( ; ;  )
     {

    	 for (unsigned int i = 0; i < numCameras; i++)
    	    {
    	      Image image;
    	      error = pCameras[i].RetrieveBuffer(&image);
    	      if (error != PGRERROR_OK)
    	        {
    	          PrintError(error);
    	          delete[] pCameras;
    	          return -1;
    	        }
    	      // Display the timestamps of the images grabbed for each camera
    	         TimeStamp timestamp = image.GetTimeStamp();
    	         cout << "Camera " << i << " - Frame " << imageCnt << " - TimeStamp [" << timestamp.cycleSeconds << " " << timestamp.cycleCount << "]"<< endl;

    	         // Save the file

    	      unsigned int res =	 0; // for initialization purposes

    	      Property shutter;
    	      shutter.type = SHUTTER;
    	      error = pCameras[i].GetProperty( &shutter );
    	      if (error != PGRERROR_OK)
    	        {
    	          PrintError( error );
    	          return -1;
    	        }


    	      error = pCameras[i].GetCameraInfo(&camInfo);
    	      if (error != PGRERROR_OK)
    	        {
    	          PrintError(error);
    	          delete[] pCameras;
    	          return -1;
    	        }

    	      char* name1;
    	      name1 = strtok(camInfo.modelName, " ");

    	       if ( strcmp("Chameleon", name1) == 0)
    	         {
    	           error = pCameras[i].ReadRegister(0x82C, &ulValue); // read the temperature register for the NIR camera only
    	           unsigned r  = createMask(20, 31); // extract the bits of interest
   	               res = r & ulValue;  // here you have already the Kelvin temperature * 10
    	           if (error != PGRERROR_OK)
    	              {
    	                PrintError( error );
    	                delete[] pCameras;
    	                return -1;
    	              }
   	              }
    	        PixelFormat pixFormat;
    	        unsigned int rows, cols, stride;
    	        image.GetDimensions( &rows, &cols, &stride, &pixFormat );

    	       // Create a unique filename

    	       std::string str;     //temporary string to hold the filename
    	       int lengthOfString1; //hold the number of characters in the string
    	       std::stringstream sstm;
    	       std::string head;
    	       if ( strcmp("Chameleon", name1) == 0)
    	         {
    	          head = "NIR";
    	         }
    	       else
    	          {
    	    	   head = "VIS";
    	          }

    	       sstm  << currentDateTime2();
    	       str = sstm.str();
    	       lengthOfString1=str.length();

    	       int lenghtsum = lengthOfString1 + 4  + 4 + 3;
    	       char filename[lenghtsum];
    	       sprintf(filename,"%s/%s-%s.raw", head.c_str(), head.c_str(), str.c_str() );
    	       //     cout << filename << endl; // uncomment for testing purposes

    	       unsigned int iImageSize = image.GetDataSize();
    	       printf( "Grabbed image %s \n", filename );
//    	                 printf( "Frame rate is %3.1f fps\n", frmRate.absValue );
    	       printf( "Shutter is %3.1f ms\n", shutter.absValue );
    	       if ( strcmp("Chameleon", name1) == 0)
    	         {
    	           cout << "Temperature is " << res / 10.0 << " K - " << res / 10.0	- 273.15 << " Celsius" << endl;
    	         }
    	       cout << "Raw Image Dimensions: " << rows  << " x " << cols << " Image Stride: " << stride << endl;
    	       cout << "Image Size: " << iImageSize << endl;

    	      // Save the image. If a file format is not passed in, then the file
    	     // extension is parsed to attempt to determine the file format.
    	       error = image.Save( filename );
    	       if (error != PGRERROR_OK)
    	         {
    	           PrintError( error );
    	           delete[] pCameras;
    	           return -1;
    	         }


      }
   	 imageCnt++;
     }

    std::stringstream ss1;
    ss1 << currentDateTime2();
    std::string st1= ss1.str();
    char pippo1[st1.length()];
    sprintf(pippo1 , "%s" , st1.c_str() );
    printf( "End time time %s \n", pippo1 );

    // disconnect the camera

    for (unsigned int  i= 0; i < numCameras; i++)
        {
          pCameras[i].StopCapture();
          pCameras[i].Disconnect();
        }

        delete[] pCameras;

    return 0;
}
