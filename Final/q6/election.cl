#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable

#define INT_MAX 4294967296

// We'll assume this is sufficiently random for the purposes of the simulation.
float random( uint x ) {
    uint value = x;
    value = (value ^ 61) ^ (value>>16);
    value *= 9;
    value ^= value << 4;
    value *= 0x27d4eb2d;
    value ^= value >> 15;
    return (float) value / (float) INT_MAX;
}

__kernel void elect(__global float3* voter, __global float3* rtn, __global int* ran_seed) {

    uint globalId = get_global_id(0);
    rtn[globalId].x = 0.0;
    rtn[globalId].y = 0.0;
    rtn[globalId].z = 0.0;

    for(int i=0;i<100000;i++)
    {
        float P = random((uint)(globalId+i+(*ran_seed)));
        if(P < voter[i].x)
        {
            rtn[globalId].x += 1;
        }
        else if(voter[i].x<=P && P<(voter[i].x+voter[i].y))
        {
            rtn[globalId].y += 1;
        }
        else
        {
            rtn[globalId].z += 1;
        }
    }

}