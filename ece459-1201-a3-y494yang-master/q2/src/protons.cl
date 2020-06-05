__kernel void computeForces(__global float3* k0, __global float4* y0, __global int* num_particles)
{
 		int globalId = get_global_id(0);
        float3 totalForces;
		totalForces.x = 0.0f;
		totalForces.y = 0.0f;
		totalForces.z = 0.0f;
		float3 final;
		final.x = 0.0f;
		final.y = 0.0f;
		final.z = 0.0f;

		if(y0[globalId].w != 1.f)
		{
			for (int i = 0; i < *num_particles; i++) 
			{
				float3 direction = y0[globalId].xyz - y0[i].xyz;
				float q1;
				if(y0[globalId].w == 1.f)
				{
					q1 = 1.60217662f * pow(10.f, -19.f);
				}
				else
				{
					q1 = -1.f*(1.60217662f * pow(10.f, -19.f));
				}
				float q2;
				if(y0[i].w == 1.f)
				{
					q2 = 1.60217662f * pow(10.f, -19.f);
				}
				else
				{
					q2 = -(1.60217662f * pow(10.f, -19.f));
				}
				float r = sqrt(pow(direction.x, 2.f) + pow(direction.y, 2.f) + pow(direction.z, 2.f));
				float3 normal = direction/r;
				final = normal * (8.99f * pow(10.f, 9.f) * q1 * q2 / pow(r, 2.f));	
				totalForces += final;
			}
        }

        k0[globalId] = totalForces;
}



__kernel void computeApproxPositions(__global float* h, __global float3* k0, __global float4* y0, __global float4* y1)
{
 	int globalId = get_global_id(0);
	int globalSize = get_global_size(0);

	float mass;
	if(y0[globalId].w == 1.f)
	{
		mass = 1.67262190 * pow(10.f, -27.f);
	}
	else
	{
		mass = 9.10938356 * pow(10.f, -31.f);
	}

	float3 f = k0[globalId];
	float3 deltaDist = f * pow(*h, 2.f) / mass;
	y1[globalId] = (float4)(y0[globalId].xyz + deltaDist, y0[globalId].w);
}

__kernel void computeBetterPositions(__global float* h, __global float3* k0, __global float3* k1, __global float4* y0, __global float4* z1)
{
	int num_groups = get_num_groups(0);
	int globalId = get_global_id(0);
	int globalSize = get_global_size(0);
	int numElement = globalSize/num_groups;

	float mass;
	if(y0[globalId].w == 1.f)
	{
		mass = 1.67262190 * pow(10.f, -27.f);
	}
	else
	{
		mass = 9.10938356 * pow(10.f, -31.f);
	}

	float3 f0 = k0[globalId];
	float3 f1 = k1[globalId];

	float3 avgForce = (f0 + f1) / 2.f;
	float3 deltaDist = avgForce * pow(*h, 2.f) / mass;
	float3 y1 = y0[globalId].xyz + deltaDist;

	z1[globalId] = (float4)(y1, y0[globalId].w);
}

__kernel void isErrorAcceptable(__global float* tolerance, __global float4* y1, __global float4* z1, __global int* accept_flag)
{
	int num_groups = get_num_groups(0);
	int globalId = get_global_id(0);
	int globalSize = get_global_size(0);
	int numElement = globalSize/num_groups;

	accept_flag[globalId] = 1;
	float3 position = z1[globalId].xyz - y1[globalId].xyz; 
	float magnitude = sqrt(pow(position.x, 2) + pow(position.y, 2) + pow(position.z, 2));
	if (magnitude > *tolerance) 
	{
			accept_flag[globalId] = 0;
	}
}
