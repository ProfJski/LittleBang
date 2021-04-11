struct particle {
    float3 pos;
    float3 vel;
    float3 accel;
    float mass;
    float radius;
};

void kernel apply_gravity(global struct particle* ob) {
    float rsquared;
    float PI=3.14159;
    float3 dF;
    float3 r;
    float mr;
    int gid=get_global_id(0);
    int obsize=get_global_size(0);
    for (int j=0; j<obsize; j++) {
        r=ob[j].pos-ob[gid].pos;
        mr=ob[j].radius+ob[gid].radius;
        mr=mr*mr;
        rsquared = dot(r,r);
        rsquared=(rsquared<mr)?mr:rsquared;
        dF=normalize(r)/rsquared;  //Can I avoid a square root here in normalize()?  Time is actually a bit slower using r*native_rsqrt()/rsquared so this is optimal so far
        ob[gid].accel+=ob[j].mass*dF;
    }
}

void kernel coll_check(global struct particle* ob, global int* CollFlag) {
    float rsquared;
    float PI=3.14159;
    float3 r;
    float sumrad;
    int gid=get_global_id(0);
    int obsize=get_global_size(0);

    for (int j=0; j<obsize; j++){
        r=ob[j].pos-ob[gid].pos;
        rsquared = dot(r,r);
        sumrad=ob[gid].radius+ob[j].radius;
        CollFlag[gid]=((j!=gid)&&(rsquared<(sumrad*sumrad)))?1:CollFlag[gid];
    }
}
