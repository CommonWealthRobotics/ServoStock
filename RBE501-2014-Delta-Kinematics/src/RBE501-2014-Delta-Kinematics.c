/*
 ============================================================================
 Name        : RBE501-2014-Delta-Kinematics.c
 Author      : Kevin Harrington, Michael DiBlasi, Bill Calabro and Tigger too
 Version     :
 Copyright   : BSD with attribution
 Description : The kinematics system for a Linear slide Delta
 ============================================================================

 This kinematics engine uses math from http://tinyurl.com/p8ce2rk

 https://docs.google.com/viewer?a=v&pid=forums&srcid=MTgyNjQwODAyMDkxNzQxMTUwNzIBMDc2NTg4NjQ0MjUxMTE1ODY5OTkBdmZiejRRR2phZjhKATQBAXYy

 */

#include <stdio.h>
#include "Kinematics.h"

float positionMatrix[4][4] = {
								{1,0,0,0},
								{0,1,0,0},
								{0,0,1,0},
								{0,0,0,1}
							  };

float jointVector[3] = {0,0,0};

float bedLevelMatrix[4][4] = {
								{1,0,0,0},
								{0,1,0,0},
								{0,0,1,0},
								{0,0,0,1}
							  };

typedef struct _DeltaConfig{
	float RodLength;
	float BaseRadius;
	float EndEffectorRadius;
	float MaxZ;
	float MinZ;
}DeltaConfig;

//static DeltaConfig defaultConfig ={203.82,//RodLength
//							150,//BaseRadius
//							40.32,//EndEffectorRadius
//							400,//MaxZ
//							0};//MinZ


int main(void) {
	puts("Running basic kinematics test"); /* prints !!!Hello World!!! */
	float cartestian [4]={ 1,0,0,0};
	//float joint [3] = {0,0,0};

	float cartestianSet[4] ={ 0,10,0,0//random values
							};
	float jointSet [3] = {0,0,0};

	if(inverseKinematics(cartestianSet, jointSet)){
		return 1;
	}

	printf("\r\nJoints A=%g B=%g C=%g",jointSet[0],jointSet[1],jointSet[2]);

	forwardKinematics(jointSet,cartestian);

	printf("\r\nSetting X=%g Y=%g Z=%g",cartestianSet[0],cartestianSet[1],cartestianSet[2]);

	printf("\r\nResult X=%g Y=%g Z=%g",cartestian[0],cartestian[1],cartestian[2]);


	// Test Cases
	printf("\r\n\r\n2D Square Test Case \r\n");
	float initialPosition[3] = {0,0,0};
	invSquare(10, initialPosition);


	return 0;
}

/**
 * Take the joint space positions and convert to a task space position
 * This should transform the output using the bed level calibration
 * Return 0 for success
 * Return error code for failure
 */
int forwardKinematics( float * currentJointPositions,
					   float * outputTaskSpacePositionMatrix
					){

	return servostock_calcForward(	currentJointPositions[0],
									currentJointPositions[1],
									currentJointPositions[2],
									&outputTaskSpacePositionMatrix[0],
									&outputTaskSpacePositionMatrix[1],
									&outputTaskSpacePositionMatrix[2]);
}

/**
 * Take the task space position and convert to a set of joint space positions
 * This should transform the input through the bed level calibration
 * Return 0 for success
 * Return error code for failure
 */
int inverseKinematics( float * currentTaskSpacePosition,
					   float *  outputJointSpacePositionVector
					){
	float X = currentTaskSpacePosition[0];
	float Y = currentTaskSpacePosition[1];
	float Z = currentTaskSpacePosition[2];

	return servostock_calcInverse(	X, Y, Z,
									&outputJointSpacePositionVector[0],
									&outputJointSpacePositionVector[1],
									&outputJointSpacePositionVector[2]);
}

/**
 * Take in the task space position and the current task space velocities
 * load the target joint velocities into the output vector
 * Return 0 for success
 * Return error code for failure
 */

int calculateJointSpaceVelocities(	float ** currentTaskSpaceVelocities,
									float ** currentTaskSpacePosition,
									float *  outputJointSpaceVelocities
								){

	return 0;
}

/**
 * Take in the current Joint space velocities and the task space position
 * load the target task space velocities into the output matrix
 * Return 0 for success
 * Return error code for failure
 */
int calculateTaskSpaceVelocity(	float *  currentJointSpaceVelocities,
								float ** currentTaskSpacePosition,
								float ** outputTaskSpaceVelocity
							){

	return 0;
}
