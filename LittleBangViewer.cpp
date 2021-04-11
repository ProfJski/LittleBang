#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <math.h>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <fstream>  //For saving images and for saving boost serialize data
#include <fenv.h>
#include <random>
#include <unistd.h> // For usleep
#include <chrono> //for timing OpenCL
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/filesystem.hpp> //For loading files from the directory

using namespace std;
using namespace std::chrono;
using namespace boost::archive;

using namespace std;


struct particle {
    Vector3 pos;
    float mass;
    float radius;
};

void drawplanets (vector<particle> &ob, Model &mymodel_small, Model &mymodel_big, bool draw_small_objects, int radscale) {
//Must call raylib BeginMode3d() prior to call to initialize camera, or pass the variable

    Color color=WHITE;

    for (unsigned int k=0; k<ob.size(); k++) {
        if (ob[k].mass > 10000) {  // Color-code planets based on their mass
            color={255,255,128,255};  //Yellow
            DrawModelEx(mymodel_big,ob[k].pos,(Vector3){1.0,0.0,0.0}, PI/2.0, (Vector3){ob[k].radius/10.0,ob[k].radius/10.0,ob[k].radius/10.0},color);
        }
        else if (ob[k].mass > 1000) {
            color={255, 64, 64, 255};  // Red
            DrawModel(mymodel_small,ob[k].pos, ob[k].radius*radscale,color);
        }
        else if (ob[k].mass > 100) {
            color={64, 255, 64, 255}; //Green
            DrawModel(mymodel_small,ob[k].pos, ob[k].radius*radscale,color);
        }
        else if (ob[k].mass > 10) {
            color={96, 96, 255, 255};  //Blue
            DrawModel(mymodel_small,ob[k].pos, ob[k].radius*radscale,color);
        }
        else {  //commenting out the below two lines great helps render speed with >100K particles
            color={255, 255, 255, 128}; //White
            if (draw_small_objects) {
                DrawModel(mymodel_small,ob[k].pos, ob[k].radius*radscale,color);;
            }
        }

    }
}
int main()
{

    bool render_freeze=false;
    bool finished=false;
    bool forwardrender=true;
    bool makemovie=false;
    bool lock_to_most_massive=false;

    int WinXsize=1200, WinYsize=900;
    float cam_radius=600.0;
    float cam_theta=0.0;
    float cam_phi=0.0;
    int movieframe=0;
    int radscale=1;

    vector <particle> ob;
    ob.reserve(1000);
    int framedelay=0;

// RayGUI variables
    bool showGUI=true;
    int toggleGroupActive=0;
    bool FrameRateSpinnerEditMode=false;
    bool draw_small_objects=false;
    bool MaxMovFrEM=false;

    //Initialize Raylib
    InitWindow(WinXsize, WinYsize, "Gravity Sim Viewer");

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    //camera.position = cam_pos;
    //camera.target = cam_targ;
    camera.position = (Vector3){0.0, 10.0, 200.0};
    camera.target=(Vector3){0.0,0.0,0.0};
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.type = CAMERA_PERSPECTIVE;

    //Image myimage=GenImagePerlinNoise(100,100,0,0,1.0);
    //Image myimage=GenImageGradientH(100,100,DARKGRAY, WHITE);

    Image temp=GenImageGradientH(100,100,(Color){0,0,0,255},(Color){255,255,255,255});
    Image temp2=GenImageGradientH(100,100,(Color){255,255,255,255},(Color){0,0,0,255});
    Image myimage=GenImageColor(200,100,(Color){255,255,255,0});
    ImageDraw(&myimage,temp,(Rectangle){0,0,100,100},(Rectangle){0,0,100,100});
    ImageDraw(&myimage,temp2,(Rectangle){0,0,100,100},(Rectangle){100,0,200,100});
    Texture2D mytexture=LoadTextureFromImage(myimage);
    UnloadImage(myimage);
    UnloadImage(temp);
    UnloadImage(temp2);

    // Create Model
    Mesh mymesh_small=GenMeshSphere(1.0,8,8); // simpler sphere mesh to push fewer vertices
    Model mymodel_small=LoadModelFromMesh(mymesh_small);
    SetMaterialTexture(&mymodel_small.materials[0],MAP_DIFFUSE,mytexture);
    Mesh mymesh_big=GenMeshSphere(10.0,16, 30);  // radius is already 10 to see if texture looks better
    Model mymodel_big=LoadModelFromMesh(mymesh_big);
    SetMaterialTexture(&mymodel_big.materials[0],MAP_DIFFUSE,mytexture);

    SetTargetFPS(30);
//Load file names to be iterated as frames below into sorted vector
    boost::filesystem::path p("fdata");
    vector<boost::filesystem::path> datafiles;
    try {
        if (exists(p)) {
            if (is_regular_file(p))
                cout << p << "is not a directory"<<endl;
            else if (is_directory(p))
            {
                for (auto&& x : boost::filesystem::directory_iterator(p))
                datafiles.push_back(x.path().relative_path());
                sort(datafiles.begin(), datafiles.end());

                //for (auto&& x : datafiles)  //Lists loaded files
                //cout << "    " << x << '\n';
            }
            else
            cout << p << " exists, but is not a regular file or directory"<<endl;
        }
        else
          cout << p << " does not exist"<<endl;
      }

  catch (const boost::filesystem::filesystem_error& ex)
  {
    cout << ex.what() << '\n';
  }

  std::vector<boost::filesystem::path>::iterator it=datafiles.begin();
  int maxframenum=datafiles.size();
  int max_movie_frame=20000;


//The render loop
    while (!WindowShouldClose()) {
        int inkey = GetKeyPressed();
        if (inkey==KEY_C) {
            cam_radius+=10.0;
            cout<<"\033[1;31mZooming OUT. Radius="<<cam_radius<<"\033[0m"<<endl;
        }
        else if (inkey==KEY_E) {
            cam_radius-=10.0;
            if (cam_radius<10.0) { cam_radius=10.0; cout<<"Min Cam Radius is 10"<<endl;}
            cout<<"\033[1;31mZooming IN. Radius="<<cam_radius<<"\033[0m"<<endl;
        }
        else if (inkey==KEY_A) {
            cam_theta-=(PI/360.0)*10.0;
            cout<<"\033[1;31mRotating NEG. Theta="<<cam_theta<<"\033[0m"<<endl;
        }
        else if (inkey==KEY_D) {
            cam_theta+=(PI/360.0)*10.0;
            cout<<"\033[1;31mRotating POS. Theta="<<cam_theta<<"\033[0m"<<endl;
        }
        else if (inkey==KEY_Y) {
            cam_phi+=(PI/360.0)*10.0;
            cout<<"\033[1;31mRotating POS. Phi="<<cam_phi<<"\033[0m"<<endl;
        }
        else if (inkey==KEY_H) {
            cam_phi-=(PI/360.0)*10.0;
            cout<<"\033[1;31mRotating NEG. Phi="<<cam_phi<<"\033[0m"<<endl;
        }

        camera.position=(Vector3){cam_radius*sin(cam_theta)*cos(cam_phi), cam_radius*sin(cam_phi), cam_radius*cos(cam_theta)*cos(cam_phi)};
        camera.position=Vector3Add(camera.position,camera.target);

        if (inkey==KEY_F) {
            render_freeze=!render_freeze;
            if (render_freeze) {
                cout<<"Render OFF"<<endl;
                }
            else {
                cout<<"Render Proceeding"<<endl;
            }
        }

        if (inkey==KEY_G) {
            showGUI=!showGUI;
        }

        if (inkey==KEY_O) {
            forwardrender=false;
            toggleGroupActive=1;
            cout<<"Rendering Backwards"<<endl;
        }
        if (inkey==KEY_P) {
            forwardrender=true;
            toggleGroupActive=0;
            cout<<"Rendering Forwards"<<endl;
        }
        if (inkey==KEY_K) {
            framedelay-=10;
            if (framedelay<0) {framedelay=0;}
            cout<<"Frame delay="<<framedelay<<" (ms)"<<endl;
        }
        if (inkey==KEY_L) {
            framedelay+=10;
            cout<<"Frame delay="<<framedelay<<" (ms)"<<endl;
        }
        if (inkey==KEY_M) {
            makemovie=!makemovie;
            if (makemovie){
                cout<<"Making movie frames"<<endl;
            }
            else {
                cout<<"Movie OFF"<<endl;
            }
        }




//Draw

        if (movieframe>max_movie_frame) {
            makemovie=false;
        }
        if (!render_freeze&&!finished){
//Load frame data
            particle newpart;
            ifstream archive{it->c_str()};
            boost::archive::binary_iarchive oa(archive);
            unsigned int archive_size;
            oa>>archive_size;
            ob.clear();
            //cout<<"Archive size"<<archive_size<<endl;
                for (unsigned int i=0;i<archive_size;++i){
                    oa>>newpart.pos.x;
                    oa>>newpart.pos.y;
                    oa>>newpart.pos.z;
                    oa>>newpart.mass;
                    oa>>newpart.radius;
                    ob.push_back(newpart);
                }

    }
            BeginDrawing();
            BeginMode3D(camera);
            ClearBackground(BLACK);
            drawplanets(ob,mymodel_small,mymodel_big, draw_small_objects,radscale);
            EndMode3D();

            //GUI code goes here
            if (showGUI){
                GuiStatusBar({0,0,120,30},it->c_str());
                render_freeze=GuiToggle({0,40,100,30},"Renderer Pause",render_freeze);
                toggleGroupActive = GuiToggleGroup((Rectangle){ 0, 80, 100, 25 }, "Forward\nBackward", toggleGroupActive);
                if (toggleGroupActive==0) {
                    forwardrender=true;
                }
                else if (toggleGroupActive==1) {
                    forwardrender=false;
                }
                GuiLabel({0,140,100,20},"Framerate delay");
                GuiSpinner({5,160,95,30},&framedelay,0,5,false);
                GuiLabel({0,200,100,20},"Enlarge small planets");
                GuiSpinner({5,220,95,30},&radscale,0,10,false);
                //cout<<"Frame Delay after spinner"<<framedelay<<endl;
                if (framedelay<0) {framedelay=0;}
                lock_to_most_massive=GuiToggle({0,270,100,30},"Lock Most Massive",lock_to_most_massive);
                draw_small_objects=GuiToggle({0,310,100,30},"Draw All Obj",draw_small_objects);
                makemovie=GuiToggle({0,350,100,30},"Make Movie Frames",makemovie);
                GuiLabel({0,390,100,20},"Max Movie Frames");
                if (GuiValueBox({0,420,100,30},&max_movie_frame,100,20000,MaxMovFrEM)) {MaxMovFrEM=!MaxMovFrEM;}

                GuiScrollBar({0,880,1200,900},(int)(it-datafiles.begin()),0,maxframenum);
            } //END SHOW GUI
            else {
                //DrawText(it->c_str(),10,10,20,WHITE);
            }
            EndDrawing();

            if (makemovie && !render_freeze) {
                char myfn[30];
                snprintf (myfn, sizeof myfn, "./movie/fr%06d.png", movieframe);
                Image image=GetScreenData();
                ExportImage(image,myfn);
                movieframe++;
                UnloadImage(image);
            }

            if (lock_to_most_massive) {
                unsigned int largest=0;
                for (unsigned int i=0; i<ob.size(); i++) {
                    if (ob[i].mass > ob[largest].mass) {
                    largest = i;
                    }
                }
                camera.target = ob[largest].pos;
            }
            else {
                camera.target=(Vector3){0.0,0.0,0.0};
            }


    if (!render_freeze&&!finished){
        usleep(10000*framedelay);
        if (forwardrender&&(it!=prev(datafiles.end()))){++it;}
        if (!forwardrender&&(it!=next(datafiles.begin()))) {--it;}
    }

    }//end render loop



    UnloadModel(mymodel_small);
    UnloadModel(mymodel_big);
    CloseWindow();
    return 0;
}
