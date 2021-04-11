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
#include <CL/cl.hpp>
#include <unistd.h> // For usleep
#include <chrono> //for timing OpenCL
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <gsl/gsl_histogram2d.h>

using namespace std;
using namespace std::chrono;
using namespace boost::archive;

int MAX_DISTANCE=5000;

struct particle {
    cl_float3 pos;
    cl_float3 vel;
    cl_float3 accel;
    float mass;
    float radius;
};

//Vector math functions for cl_float3s
cl_float3 clf3_add(cl_float3 a, cl_float3 b) {
    cl_float3 output;
    output.s[0]=a.s[0]+b.s[0];
    output.s[1]=a.s[1]+b.s[1];
    output.s[2]=a.s[2]+b.s[2];
    return output;
}

cl_float3 clf3_subtract(cl_float3 a, cl_float3 b) {
    cl_float3 output;
    output.s[0]=a.s[0]-b.s[0];
    output.s[1]=a.s[1]-b.s[1];
    output.s[2]=a.s[2]-b.s[2];
    return output;
}

cl_float3 clf3_scale(cl_float3 a, float s) {
    cl_float3 output;
    output.s[0]=a.s[0]*s;
    output.s[1]=a.s[1]*s;
    output.s[2]=a.s[2]*s;
    return output;
}

float clf3_dot(cl_float3 a, cl_float3 b) {
    return (a.s[0]*b.s[0]+a.s[1]*b.s[1]+a.s[2]*b.s[2]);
}

float clf3_length(cl_float3 a) {
    return sqrt(clf3_dot(a,a));
}

cl_float3 clf3_normalize(cl_float3 a) {
    return clf3_scale(a,1.0/clf3_length(a));
}

bool vsort_mass (particle I1, particle I2 ) {
    return (I1.mass<I2.mass);
}

bool Cull (const particle & p) {return (p.mass==0.0);}

void drawplanets (vector<particle> &ob, Model &mymodel_small, Model &mymodel_big) {
//Must call raylib BeginMode3d() prior to call to initialize camera, or pass the variable

    Color color=WHITE;

    for (unsigned int k=0; k<ob.size(); k++) {
        if (ob[k].mass > 10000) {  // Color-code planets based on their mass
            color={255,255,128,255};  //Yellow
            DrawModelEx(mymodel_big,(Vector3){ob[k].pos.s[0],ob[k].pos.s[1],ob[k].pos.s[2]},(Vector3){1.0,0.0,0.0}, PI/2.0, (Vector3){ob[k].radius/10.0f,ob[k].radius/10.0f,ob[k].radius/10.0f},color);
        }
        else if (ob[k].mass > 1000) {
            color={255, 64, 64, 255};  // Red
            DrawModel(mymodel_small,(Vector3){ob[k].pos.s[0],ob[k].pos.s[1],ob[k].pos.s[2]}, ob[k].radius,color);
        }
        else if (ob[k].mass > 100) {
            color={64, 255, 64, 255}; //Green
            DrawModel(mymodel_small,(Vector3){ob[k].pos.s[0],ob[k].pos.s[1],ob[k].pos.s[2]}, ob[k].radius,color);
        }
        else if (ob[k].mass > 10) {
            color={96, 96, 255, 255};  //Blue
            DrawModel(mymodel_small,(Vector3){ob[k].pos.s[0],ob[k].pos.s[1],ob[k].pos.s[2]}, ob[k].radius,color);
        }
        else {
            color={255, 255, 255, 255}; //White
            DrawModel(mymodel_small,(Vector3){ob[k].pos.s[0],ob[k].pos.s[1],ob[k].pos.s[2]}, ob[k].radius,color);;
        }

    }
}

void update_apply_accel(vector<particle> &ob, bool &ttc) {
    for (unsigned int i=0; i<ob.size(); i++) {
        ob[i].vel.s[0] += ob[i].accel.s[0];  //Leaving componentwise for now in case I want to slow down time constant for high vel
        ob[i].vel.s[1] += ob[i].accel.s[1];
        ob[i].vel.s[2] += ob[i].accel.s[2];
        ob[i].pos.s[0] += ob[i].vel.s[0];
        ob[i].pos.s[1] += ob[i].vel.s[1];
        ob[i].pos.s[2] += ob[i].vel.s[2];

        if (fabs(ob[i].pos.s[0]) > MAX_DISTANCE) {
            ttc = true;
        }
        if (fabs(ob[i].pos.s[1] > MAX_DISTANCE)) {
            ttc = true;
        }
        if (fabs(ob[i].pos.s[2] > MAX_DISTANCE)) {
            ttc = true;
        }
    }
}

void cull_distant_objects(vector<particle> &ob) {
    float distsq=0;
    float max_distsq = MAX_DISTANCE * MAX_DISTANCE;
    for (unsigned int i=0; i<ob.size(); i++) {
        distsq = clf3_dot(ob[i].pos,ob[i].pos);
        if ( distsq > max_distsq ){
            ob[i].mass = 0;
        }
    }
}

void clear_accel_array(vector<particle> &ob){
        //Clear old values of acceleration
    for (unsigned int i=0; i<ob.size(); i++) {
        ob[i].accel = {0.0,0.0,0.0};
    }
}

void collision_routine(vector<particle> &ob, vector<particle> &newones, int* CollFlag) {

    float rsquared;
    for (unsigned int i=0; i<(ob.size()-1); i++) {  //ob.size-1 because we don't need to check the last one against itself
        if (CollFlag[i]==1) {
            for (unsigned int j=i+1; j<ob.size(); j++) {  //j=1+1 to check from ob[i]+1 to end, since collisions with prior ob[i] have already been detected
                if (CollFlag[j]==1) {
                    if ((ob[i].mass!=0)&&(ob[j].mass!=0)) { //needed in case a mass was set to zero earlier in iterations
                    rsquared = clf3_dot(clf3_subtract(ob[j].pos,ob[i].pos),clf3_subtract(ob[j].pos,ob[i].pos));
                        if (rsquared < (ob[i].radius+ob[j].radius)*(ob[i].radius+ob[j].radius)) {
                            if ((ob[i].mass<30)||(ob[j].mass<30)||(ob[i].mass/ob[j].mass > 5)||(ob[j].mass/ob[i].mass > 5)) { //masses<30 don't shatter, just stick.  Much smaller masses stick to larger.
                            //if ((ob[i].mass<30)||(ob[j].mass<30)) { //
                            //if (true) {
                                ob[i].vel=clf3_scale(clf3_add(clf3_scale(ob[i].vel,ob[i].mass),clf3_scale(ob[j].vel,ob[j].mass)),1.0/(ob[i].mass+ob[j].mass)); //conserve momentum in collision ob[i].vel=(m1*v1+m2*v2)/(m1+m2)
                                ob[i].mass+=ob[j].mass;
                                ob[i].radius=cbrt(3.0*ob[i].mass/(4.0*PI));
                                ob[j].mass=0;
                            }

                            else {  //masses collide, first mass deflects, second breaks in two
                                //cout<<"Collision deflection actually happened"<<endl;

                                //Do two-body off-center elastic collision vector kinematics first for deflection
                                cl_float3 NormalizedCenterLine=clf3_normalize(clf3_subtract(ob[j].pos,ob[i].pos));
                                float a1,a2,P;
                                a1=clf3_dot(ob[i].vel,NormalizedCenterLine);
                                a2=clf3_dot(ob[j].vel,NormalizedCenterLine);
                                P=2*(a1-a2)/(ob[i].mass+ob[j].mass);
                                ob[i].vel=clf3_subtract(ob[i].vel,clf3_scale(NormalizedCenterLine,P*(ob[j].mass)));
                                ob[j].vel=clf3_add(ob[j].vel,clf3_scale(NormalizedCenterLine,P*(ob[i].mass)));


                                 //then break ob[i]
                                 //the mass!=0 checks should rule it out of all further interactions.  the newly generated particles will not be interacting this frame -- is this a problem?
                                 //Doing the mass=0 as flag so as to not invalidate iterator by deleting object mid-routine

                                particle newpart1, newpart2;
                                cl_float3 nv1, nv2;
                                float angle=PI/4.0;

                                //matrix rotation of original vel by angle rad around z
                                nv1.s[0]=ob[i].vel.s[0]*cos(angle)+ob[i].vel.s[1]*sin(angle); //x=x*cos(a)+y*sin(a)
                                nv1.s[1]=ob[i].vel.s[0]*-1*sin(angle)+ob[i].vel.s[1]*cos(angle); //y=-x*sin(a)+y*cos(a)
                                nv1.s[2]=ob[i].vel.s[2]; //z dimension unchanged

                                nv2.s[0]=ob[i].vel.s[0]*cos(-1.0*angle)+ob[i].vel.s[1]*sin(-1.0*angle);
                                nv2.s[1]=ob[i].vel.s[0]*-1*sin(-1.0*angle)+ob[i].vel.s[1]*cos(-1.0*angle);
                                nv2.s[2]=ob[i].vel.s[2]; //z dimension unchanged

                                //define newpart1
                                newpart1.vel=nv1;
                                newpart1.pos=clf3_add(ob[i].pos,newpart1.vel); //advance position so doesn't immediately re-collide
                                newpart1.accel={0.0,0.0,0.0};
                                newpart1.mass=(ob[i].mass)/2.0;
                                newpart1.radius=cbrt(3.0*newpart1.mass/(4.0*PI));

                                //define newpart2
                                newpart2.vel=nv2;
                                newpart2.pos=clf3_add(ob[i].pos,newpart2.vel); //advance position so doesn't immediately re-collide
                                newpart2.accel={0.0,0.0,0.0};
                                newpart2.mass=(ob[i].mass)/2.0;
                                newpart2.radius=cbrt(3.0*newpart2.mass/(4.0*PI));

                                newones.push_back(newpart1);
                                newones.push_back(newpart2);

                                //prepare ob[i] for cull
                                ob[i].mass=0;

                            } //end of fragment generation
                        } //end of collision-detection if
                    } //end mass=0 if
                } //end of Coll-check IF loop inside j
            } //end of j-loop
        } //end of Coll-check IF loop inside i
    } //end of consolidation-collision big loop
} //end of function

Color MapColorizer(float plotval) {
    Color retcolor={0,0,0,255};
    if ((plotval>0)&&(plotval<10)){
        retcolor=(Color){128,0,0,64};
    }
    if ((plotval>10)&&(plotval<100)){
        retcolor=(Color){0,0,255,255};
    }
    if ((plotval>100)&&(plotval<1000)) {
        retcolor=(Color){32,255,32,255};
    }
    if ((plotval>=1000)&&(plotval<10000)) {
        retcolor=(Color){255,64,64,255};
    }
    if (plotval>=10000){
        retcolor=(Color){255,64,255,255};
    }

return retcolor;
}

void fancymap(vector<particle> &ob, int WinXsize, int MAXDISTANCE) {
//must be called within a Draw routine
                Color retcolor;
                DrawRectangle(WinXsize-200, 0, 200, 200, (Color){0,0,0,128});
                float dx, dy;
                for (unsigned int i=0; i<=ob.size(); i++) {
                    dx = WinXsize-100.0+(100.0*ob[i].pos.s[0])/(float)MAX_DISTANCE;
                    dy = 100.0-(100.0*ob[i].pos.s[1])/(float)MAX_DISTANCE;
                    //cout<<"Dx="<<dx<<" Dy="<<dy<<endl;
                    if (ob[i].mass<=0){
                        retcolor=(Color){0,0,0,0};
                    }
                    if ((ob[i].mass>0)&&(ob[i].mass<10)){
                        retcolor=(Color){128,32,32,32};
                        DrawPixel((int)dx, (int)dy,retcolor);
                    }
                    if ((ob[i].mass>10)&&(ob[i].mass<100)){
                        retcolor=(Color){64,0,255,64};
                        DrawPixel((int)dx, (int)dy,retcolor);
                    }
                    if ((ob[i].mass>100)&&(ob[i].mass<1000)) {
                        retcolor=(Color){32,255,32,200};
                        DrawCircle((int)dx,(int)dy,2,retcolor);
                    }
                    if ((ob[i].mass>=1000)&&(ob[i].mass<10000)) {
                        retcolor=(Color){230,32,32,190};
                        DrawPixel((int)dx, (int)dy,retcolor);
                        //DrawCircle((int)dx,(int)dy,3,retcolor);
                    }
                    if (ob[i].mass>=10000){
                        retcolor=(Color){255,255,0,255};
                        //DrawCircle((int)dx,(int)dy,4,retcolor);
                        DrawCircleGradient((int)dx,(int)dy,8,YELLOW,BLACK);
                    }
                }
}

// ************** MAIN ************


int main(int argc, char* argv[])
{

    int MAXOBJ = 25000;
    bool done = false;
    bool movie = false;
    bool spew_on = false;
    bool time_to_cull = false;
    bool lock_to_most_massive = false;
    bool render_freeze = true;
    bool plot_KE = true;
    bool plot_MAP = true;
    bool save_state = false;
    bool backup_state = false;
    bool load_state = false;
    bool display_fastest = false;
    bool init_screen_complete = false;

    int max_movie_frame = 10000;
    int max_frame = 20000;
    int SPEWSTOP = 4000;

    vector <particle> ob;
    ob.reserve(MAXOBJ);
    vector <particle> newones;
    particle ip;


    int WinXsize=1200, WinYsize=900;
    int framedelay = 0;
    int framedisplay = 1;
    int frame=0;

    int largest=0;
    float KE=0.0;
    vector<float> KEvec;
    KEvec.reserve(WinXsize);
    Image image;

    float cam_radius=1000.0;
    float cam_theta=0.0;
    float cam_phi=0.0;

    int maxradius=300; //For initial particle distribution
    float initvel=7.0;
    float initmass=1.0;


    high_resolution_clock::time_point t1, t2;

    cout<<"Size of particle struct="<<sizeof(particle)<<endl;


//Initialize Raylib
    InitWindow(WinXsize, WinYsize, "Gravity Sim");
    SetWindowPosition(500,50);

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = (Vector3){0.0, 10.0, 200.0};
    camera.target=(Vector3){0.0,0.0,0.0};
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.type = CAMERA_PERSPECTIVE;

    Image temp=GenImageGradientH(100,100,(Color){128,128,128,255},(Color){255,255,255,255});
    Image temp2=GenImageGradientH(100,100,(Color){255,255,255,255},(Color){128,128,128,255});
    Image myimage=GenImageColor(200,100,(Color){255,255,255,255});

    ImageDraw(&myimage,temp,(Rectangle){0,0,100,100},(Rectangle){0,0,100,100});
    ImageDraw(&myimage,temp2,(Rectangle){0,0,100,100},(Rectangle){100,0,100,100});
    Texture2D mytexture=LoadTextureFromImage(myimage);
    //GenTextureMipmaps(&mytexture);
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

    //Start up GUI
    Image splash=LoadImage("splash.png");
    ImageColorTint(&splash,{255,255,255,128});
    Texture2D splashscreen=LoadTextureFromImage(splash);
//    Texture2D splashscreen=LoadTextureFromImage(LoadImage("splash.png"));
    Texture2D raylib=LoadTexture("raylib_128x128.png");
    Texture2D openCL=LoadTexture("openCL-logo.png");
    Texture2D blogo=LoadTexture("boost-logo.png");

    //GUI Variables
    bool MaxObjEM=false;
    bool MaxDistanceEM=false;
    bool MaxRadiusEM=false;
    bool MaxRendEM=false;
    bool MaxMovFrEM=false;
    bool InitVelEM=false;
    bool InitMassEM=false;
    bool ShowHelp=false;
    int IV=7;
    int IM=1;

    while (!init_screen_complete&&!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(splashscreen,0,0,WHITE);
        DrawText("Little Bang:",100,10,30,SKYBLUE);
        DrawText("OpenCL Gravity Simulator",60,40,25,SKYBLUE);

        DrawRectangle(25,100,400,260,{0,0,0,128});
        GuiGroupBox({25,100,400,260},"Initialization Parameters");
        GuiLabel({35,105,120,20},"Max Objects");
        if (GuiValueBox({35,135,120,30},&MAXOBJ,16,500000,MaxObjEM)) {MaxObjEM=!MaxObjEM;}
        GuiLabel({165,105,120,20},"Max Distance");
        if (GuiValueBox({165,135,120,30},&MAX_DISTANCE,1000,50000,MaxDistanceEM)) {MaxDistanceEM=!MaxDistanceEM;}
        GuiLabel({295,105,120,20},"Max Frames");
        if (GuiValueBox({295,135,120,30},&max_frame,1,50000,MaxRendEM)) {MaxRendEM=!MaxRendEM;}

        GuiLabel({35,190,120,20},"Max Radius");
        if (GuiValueBox({35,220,120,30},&maxradius,100,10000,MaxRadiusEM)) {MaxRadiusEM=!MaxRadiusEM;}
        GuiLabel({165,190,120,20},"Initial velocity");
        if (GuiValueBox({165,220,120,30},&IV,0,100,InitVelEM)) {InitVelEM=!InitVelEM;}
        GuiLabel({295,190,120,20},"Initial mass");
        if (GuiValueBox({295,220,120,30},&IM,1,10,InitMassEM)) {InitMassEM=!InitMassEM;}

        plot_KE=GuiToggle({35,300,100,30},"Plot System KE",plot_KE);
        plot_MAP=GuiToggle({145,300,100,30},"Show minimap",plot_MAP);
        display_fastest=GuiToggle({255,300,100,30},"Show max vel",display_fastest);

        DrawRectangle(490,100,500,65,{0,0,0,128});
        GuiGroupBox({490,100,500,65},"Movie Options");
        movie=GuiToggle({500,125,100,30},"Make movie",movie);
        GuiLabel({610,100,120,20},"Max Movie Frames");
        if (GuiValueBox({610,125,120,30},&max_movie_frame,10,50000,MaxMovFrEM)) {MaxMovFrEM=!MaxMovFrEM;}
        DrawText("Stop movie at this frame",740,135,10,BLACK);

        DrawRectangle(490,180,500,140,{0,0,0,128});
        GuiGroupBox({490,180,500,140}, "Save Frame Data Options");
        save_state=GuiToggle({500,190,100,30},"Save viewer data",save_state);
        DrawText("Saves position data for each frame for LB Viewer",610,200,10,SKYBLUE);
        backup_state=GuiToggle({500,230,100,30},"Save resume data",backup_state);
        DrawText("Saves complete state of last frame rendered for resuming",610,240,10,SKYBLUE);
        load_state=GuiToggle({500,270,100,30},"Resume last state",load_state);
        DrawText("Start by loading resume data",610,280,10,SKYBLUE);

        ShowHelp=GuiToggle({500,330,100,30},"Display Help",ShowHelp);

        render_freeze=GuiToggle({150,400,100,30},"Start paused",render_freeze);
        init_screen_complete=GuiToggle({150,440,100,50},"Begin!",init_screen_complete);

        //cout<<"Help="<<ShowHelp<<endl;
        if (ShowHelp){
            DrawRectangle(35,510,955,310,{0,0,0,128});
            GuiGroupBox({35,510,955,310},"Help!");
            DrawText("Initialization Options:",40,520,10,RAYWHITE);
            DrawText("Max Objects = Total number of particles system at the start.  Compute time ~ Objects^2.", 50, 540, 10, RAYWHITE);
            DrawText("As particles consolidate, the total number of objects decreases over time and frame rates increase.",50,550,10,RAYWHITE);
            DrawText("Max Distance = Particles further from the center will be removed.  Helps keep compute times low when particles drift off.", 50,570,10,RAYWHITE);
            DrawText("Max Frames = Program will exit at this frame number or when particles=0.  Use to limit long runs.", 50, 590,10, RAYWHITE);
            DrawText("Max Radius = Outer bound of initial spherical cloud of particles", 50, 610,10, RAYWHITE);
            DrawText("Initial Velocity = Uniform for all particles in cloud.  Radially outward", 50, 630, 10, RAYWHITE);
            DrawText("Set low, the cloud will collapse quickly. Set high, it will drift apart.  In between, nice clusters form.  The sweet spot depends on cloud density", 50, 640,10,RAYWHITE);
            DrawText("Initial Mass = Basic unit is 1.", 50, 660, 10, RAYWHITE);
            DrawText("Move Options:",40,680,10,RAYWHITE);
            DrawText("Make movie = Performs .png screen capture once per frame.  If you wish to make a movie from a different perspective, use the LB viewer.",50,700,10,RAYWHITE);
            DrawText("Max movie frames = Automatically stops the movie at this frame.  Helps not to fill your hard disk with gigs of .pngs when unattended.",50,720,10,RAYWHITE);
            DrawText("Frame Data Options:",40, 740,10,RAYWHITE);
            DrawText("Save viewer data = Saves particle position and mass data for every frame. Needed for LB viewer. Saves one file for each frame. Can generate many MB of files!",50,760,10,RAYWHITE);
            DrawText("Save resume data = Saves complete state of system for last frame only. Select if you wish to stop the program and resume later.",50,780,10,RAYWHITE);
            DrawText("Resume last state = Select to start from where you left off.  Ignores initialization settings and loads your saved system.",50,800,10,RAYWHITE);
        }
        else {
            DrawRectangle(40,705,420,135,{128,128,128,128});
            GuiGroupBox({35,700,955,145},"Powered by:");
            DrawTexture(raylib,45,710,WHITE);
            DrawTexture(openCL,185,710,WHITE);
            DrawTexture(blogo,325,710,WHITE);
        }

        EndDrawing();


    }
    //convert our int GUI values to floats
    initvel=(float)IV;
    initmass=(float)IM;
    UnloadImage(splash);
    UnloadTexture(splashscreen);
    UnloadTexture(openCL);
    UnloadTexture(raylib);



//Init OpenCL
        //get all platforms (drivers)
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if(all_platforms.size()==0){
        std::cout<<" No platforms found. Check OpenCL installation!\n";
        exit(1);
    }
    cl::Platform default_platform=all_platforms[0];
    std::cout << "Using platform: "<<default_platform.getInfo<CL_PLATFORM_NAME>()<<"\n";

    //get default device of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if(all_devices.size()==0){
        std::cout<<" No devices found. Check OpenCL installation!\n";
        exit(1);
    }
    cl::Device default_device=all_devices[0];
    std::cout<< "Using device: "<<default_device.getInfo<CL_DEVICE_NAME>()<<"\n";


    cl::Context context({default_device});

    //Load kernel from file
    ifstream t("./kernel.cl");
    if (!t) { cout << "Error Opening Kernel Source file\n"; exit(-1); }
    std::string kSrc((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
    cl::Program::Sources sources(1, make_pair(kSrc.c_str(), kSrc.length()));

    cl::Program program(context,sources);
    if(program.build({default_device})!=CL_SUCCESS){
        std::cout<<" Error building: "<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device)<<"\n";
        exit(1);
    }
    //create queue to which we will push commands for the device.
    cl::CommandQueue myqueue(context,default_device);

    cl::Kernel gravkernel=cl::Kernel(program,"apply_gravity", NULL);
    cl::Kernel collkernel=cl::Kernel(program,"coll_check", NULL);

    //Define buffers
    cl::Buffer buffer_CollFlag(context,CL_MEM_READ_WRITE,sizeof(int)*MAXOBJ);
    cl::Buffer buffer_PART(context,CL_MEM_READ_WRITE,sizeof(particle)*MAXOBJ);
    cl::Buffer buffer_PART_OUT(context,CL_MEM_READ_WRITE,sizeof(particle)*MAXOBJ);

    int *CollFlag=new int[MAXOBJ];


// Initialize gravity objects

        default_random_engine generator;
        uniform_real_distribution<float> dist1(-1.0*maxradius,1.0*maxradius);
        float randrad, rx, ry, rz;

        for (int i=0; i<MAXOBJ; i++){
            rx=dist1(generator);
            ry=dist1(generator);
            rz=dist1(generator);
            randrad=sqrt(rx*rx+ry*ry+rz*rz);
            while (randrad > maxradius) {
                rx=dist1(generator);
                ry=dist1(generator);
                rz=dist1(generator);
                randrad=sqrt(rx*rx+ry*ry+rz*rz);
                //cout<<"r="<<randrad<<" rx="<<rx<<" ry="<<ry<<" rz="<<rz<<endl;
            }
            ip.pos={rx, ry, rz};
            ip.vel.s[0]=initvel*rx/randrad;
            ip.vel.s[1]=initvel*ry/randrad;
            ip.vel.s[2]=initvel*rz/randrad;
            ip.accel={0.0,0.0,0.0};
            ip.mass = initmass;
            ip.radius=cbrt(3.0*ip.mass/(4*PI));
            ob.push_back(ip);
        }

        for (unsigned int i=0; i<ob.size(); i++) {
            KE+=0.5*ob[i].mass*clf3_dot(ob[i].vel,ob[i].vel);
        }
        cout<<"Total KE: "<<KE<<endl;
        KEvec.push_back(KE);

       // collision_routine(ob, newones); //call collision routine once after initialization in case two particles were spawned in the same place
       // cout<<"Done initial coll check"<<endl;

//The render loop
        while (!(frame>max_frame||done||(ob.size()<1)||WindowShouldClose())) {
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

        if (inkey==KEY_O) {
            if (framedelay>10) {
                framedelay-=10;
            }
            else {
                framedelay=0;
            }
            cout<<"Frame Delay="<<framedelay<<" (ms)"<<endl;
        }

        if (inkey==KEY_P) {
            framedelay+=10;
            cout<<"Frame Delay="<<framedelay<<" (ms)"<<endl;
        }

        if (inkey==KEY_ONE) {
            framedisplay=1;
        }
        else if (inkey==KEY_TWO) {
            framedisplay=10;
        }
        else if (inkey==KEY_THREE) {
            framedisplay=100;
        }
        else if (inkey==KEY_FOUR) {
            framedisplay=1000;
        }

        if (inkey==KEY_L) {
            lock_to_most_massive=!lock_to_most_massive;
         }

        if (lock_to_most_massive) {
            largest=0;
            for (unsigned int i=0; i<ob.size(); i++) {
                if (ob[i].mass > ob[largest].mass) {
                    largest = i;
                }
            camera.target = (Vector3){ob[largest].pos.s[0],ob[largest].pos.s[1],ob[largest].pos.s[2]};
            }
        }
        else {
            camera.target=(Vector3){0.0,0.0,0.0};
        }

        if (inkey==KEY_V) {
            display_fastest=!display_fastest;
         }

        if (inkey==KEY_K) {
            plot_KE=!plot_KE;
        }

        if (inkey==KEY_U) {
            plot_MAP=!plot_MAP;
        }

        if (inkey==KEY_M) {
            movie=!movie;
        }

        if (inkey==KEY_NINE) {
            save_state=!save_state;
        }

        if (inkey==KEY_SEVEN) {
            backup_state=true;
        }

        if (inkey==KEY_EIGHT) {
            load_state=true;
        }

        if (load_state){
            particle newpart;
            ifstream backup{"backup.bin"};
            boost::archive::binary_iarchive oa(backup);
            unsigned int archive_size;
            oa>>archive_size;
            oa>>frame;
            ob.clear();
            ob.reserve(archive_size);
            for (unsigned int i=0;i<archive_size;++i){
                oa>>newpart.pos.s[0];
                oa>>newpart.pos.s[1];
                oa>>newpart.pos.s[2];
                oa>>newpart.vel.s[0];
                oa>>newpart.vel.s[1];
                oa>>newpart.vel.s[2];
                oa>>newpart.mass;
                oa>>newpart.radius;
                ob.push_back(newpart);
            }
            cout<<"Restored saved state"<<endl;
            load_state=false;
        }


// Plot the points


            BeginDrawing();
            BeginMode3D(camera);
            ClearBackground(BLACK);
            //DrawGizmo({0.0,0.0,0.0});
            drawplanets(ob,mymodel_small,mymodel_big);
            EndMode3D();

            if (plot_KE && !render_freeze) {
                KE=0.0;
                for (unsigned int i=0; i<ob.size(); i++) {
                    KE+=0.5*ob[i].mass*clf3_dot(ob[i].vel,ob[i].vel);
                }
                KEvec.push_back(KE);

                DrawRectangle(0,WinYsize-100,WinXsize,100,(Color){128,128,128,64});
                int maxsize=KEvec.size();
                if (maxsize>WinXsize) {maxsize=WinXsize;}
                    int maxKE=1.0;
                    for (unsigned int i=KEvec.size()-maxsize; i<KEvec.size(); i++) {
                        if (KEvec[i]>maxKE) {maxKE=KEvec[i];}
                    }
                    int ox=0;
                    for (unsigned int i=KEvec.size()-maxsize; i<KEvec.size(); i++) {
                        DrawPixel(ox,WinYsize-(int)floor(100*KEvec[i]/maxKE),YELLOW);
                        ox++;
                    }
            }

            if (plot_MAP && !render_freeze) {
                fancymap(ob,WinXsize,MAX_DISTANCE);

/*
                DrawRectangle(WinXsize-200, 0, 200, 200, (Color){128,128,128,64});
                float dx, dy;
                for (unsigned int i=0; i<ob.size(); i++) {
                    dx = WinXsize-100.0+(100.0*ob[i].pos.s[0])/(float)MAX_DISTANCE;
                    dy = 100.0-(100.0*ob[i].pos.s[1])/(float)MAX_DISTANCE;
                    //cout<<"Dx="<<dx<<" Dy="<<dy<<endl;
                    DrawPixel((int)dx, (int)dy, (Color){255,0,0,64} );
                }
*/
            }

            EndDrawing();

            if (movie && (frame < max_movie_frame) && !render_freeze) {
                char myfn[30];
                snprintf (myfn, sizeof myfn, "./movie/fr%06d.png", frame);
                image=GetScreenData();
                ExportImage(image,myfn);
            }

            if (save_state && !render_freeze) {
                char myfn[30];
                snprintf (myfn, sizeof myfn, "./fdata/fd%06d.bin", frame);
                ofstream archive{myfn};
                boost::archive::binary_oarchive oa(archive);
                int archive_size=ob.size();
                oa<<archive_size;
                for (unsigned int i=0;i<ob.size();++i){
                    oa<<ob[i].pos.s[0];
                    oa<<ob[i].pos.s[1];
                    oa<<ob[i].pos.s[2];
                    oa<<ob[i].mass;
                    oa<<ob[i].radius;
                }

                //destructors for ofstream and text_oarchive called automatically when they go out of scope
            }

            if (backup_state && !render_freeze){
                string fn="backup.bin";
                ofstream backup{fn.c_str()};
                boost::archive::binary_oarchive oa(backup);
                unsigned int archive_size=ob.size();
                oa<<archive_size;
                oa<<frame;
                for (unsigned int i=0;i<ob.size();++i){
                    oa<<ob[i].pos.s[0];
                    oa<<ob[i].pos.s[1];
                    oa<<ob[i].pos.s[2];
                    oa<<ob[i].vel.s[0];
                    oa<<ob[i].vel.s[1];
                    oa<<ob[i].vel.s[2];
                    oa<<ob[i].mass;
                    oa<<ob[i].radius;
                }
                //cout<<"Backed up current state at frame "<<frame<<endl;
                //backup_state=false;
            }

               usleep(framedelay*1000.0);

//Update work goes here

//Calculate force on each mass from every other one

            if (!render_freeze) {

                t1 = high_resolution_clock::now();  //START TIMER
                clear_accel_array(ob);

                //CL Way
                myqueue.enqueueWriteBuffer(buffer_PART,CL_TRUE,0,sizeof(particle)*ob.size(),&ob[0],NULL,NULL);

                //Run the kernel
                gravkernel.setArg(0,buffer_PART);
                myqueue.enqueueNDRangeKernel(gravkernel,cl::NullRange,cl::NDRange(ob.size()),cl::NullRange);

                //Get data back out
                myqueue.enqueueReadBuffer(buffer_PART,CL_TRUE,0,sizeof(particle)*ob.size(),&ob[0], NULL, NULL);

                //cout<<"Done OpenCL apply_grav"<<endl;


//Update new velocity and position for every particle
//Also do simple check to trigger whether cull distant objects should run
                update_apply_accel(ob, time_to_cull);

//Consolidate particles that have collided
                //CL Way
                //cout<<"Starting CL coll_check"<<endl;

                for (unsigned int i=0; i<ob.size(); i++) {  // Reset CollFlag array
                    CollFlag[i]=0;
                }
                myqueue.enqueueWriteBuffer(buffer_PART,CL_TRUE,0,sizeof(particle)*ob.size(),&ob[0],NULL,NULL);
                myqueue.enqueueWriteBuffer(buffer_CollFlag,CL_TRUE,0,sizeof(int)*ob.size(),CollFlag,NULL,NULL);

                //Run the kernel
                collkernel.setArg(0,buffer_PART);
                collkernel.setArg(1,buffer_CollFlag);

                myqueue.enqueueNDRangeKernel(collkernel,cl::NullRange,cl::NDRange(ob.size()),cl::NullRange);

                //Get data back out
                myqueue.enqueueReadBuffer(buffer_CollFlag,CL_TRUE,0,sizeof(int)*ob.size(),CollFlag,NULL,NULL);

                //cout<<"Done OpenCL coll check.  Starting C++ collision routine"<<endl;
                collision_routine(ob, newones, CollFlag);
                //cout<<"Finished C++ Collision Routine"<<endl;


// cull distant objects
                if (time_to_cull){
                    cull_distant_objects(ob);
                }

//Cull particles of mass zero, which are particles that have been deleted after consolidation or collision
//Then insert new particles which are fragments created by collision, then clear newones vector for next loop
                ob.erase(remove_if(ob.begin(), ob.end(), Cull), ob.end());
                ob.insert(ob.end(),newones.begin(),newones.end());
                newones.clear();

//Automatic spew stop
                if (frame>SPEWSTOP) {
                    spew_on = false;
                }

//Spew new particles
                if (spew_on){
                    //spew(newones);
                    ob.insert(ob.end(),newones.begin(),newones.end());
                    newones.clear();
                }


    t2 = high_resolution_clock::now();   // STOP TIMER
    auto duration = duration_cast<milliseconds>( t2 - t1 ).count();

 //Print frame number and update frame
                if (frame%framedisplay==0) {
                    cout<<"Duration="<<duration<<" ms"<<endl;
                    cout<<"Obj Size: "<<ob.size()<<" Frame:"<<frame<<endl;
                    if (plot_KE){
                        cout<<"Total KE: "<<KE<<endl;
                    }
                    if (display_fastest) {
                        unsigned int fastest=0;
                        float fastest_speed=0.0;
                        for (unsigned int i=0; i<ob.size(); i++) {
                            if (clf3_dot(ob[i].vel,ob[i].vel) > fastest_speed) {
                                fastest = i;
                                fastest_speed=clf3_dot(ob[fastest].vel,ob[fastest].vel);
                            }
                        }
                        cout<<"Fastest particle #"<<fastest<<" travels at"<<sqrt(fastest_speed)<<endl;
                    }
                }

                frame++;
                //render_freeze=true;  //Unset to pause after each frame


            } //End of Render freeze IF
        } // End of while render loop

//Print particle data when done
            largest=0;
            float totalmass=0;
            sort(ob.begin(),ob.end(),vsort_mass);
            for (unsigned int i=0; i<ob.size(); i++) {
            //printf("Obj %2i PosX %5.0f PosY %5.0f PosZ %5.0f VelX %2.2f VelY %2.2f VelZ %2.2f Mass %f \n", i, ob[i].pos.s[0], ob[i].pos.s[1], ob[i].pos.s[2], ob[i].vel.s[0], ob[i].vel.s[1], ob[i].vel.s[2], ob[i].mass);
            //cout<<"Mass:"<<ob[i].mass<<" Radius:"<<ob[i].radius;
            totalmass+=ob[i].mass;
            if (ob[i].mass > ob[largest].mass)
                {
                largest=i;
                }
            }
            cout<<"Largest mass object #"<<largest<<"weighs"<<ob[largest].mass<<"Total mass of system:"<<totalmass<<endl;


    //Free large array memory allocated on heap
    delete[] CollFlag;
    //Unload raylib models
    UnloadModel(mymodel_small);
    UnloadModel(mymodel_big);
    CloseWindow();
    return 0;
} // End of main
