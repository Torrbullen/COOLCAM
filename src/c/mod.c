#include "pad.h"
#include "types.h"
#include "math.h"

/////////////////////////////////////////////////////////////////////

#define NO_INPUT 0
#define NEUTRAL 0x80
#define DEADTHRESH 10

#define LOOK_AT_MODE_NONE	  0 
#define LOOK_AT_MODE_TARGET   1
#define LOOK_AT_MODE_RATCHET  2

// In-game camera struct. Used with the extern called camera
typedef struct {
	Vec4 forward;
	Vec4 right;
	Vec4 up;
	Vec4 pos;
} Camera;

// Used for saving camera position
typedef struct {
	int saved;
	Camera cam;
	float yaw;
	float pitch;
} Savepos;

// Use this struct so that we know that these things are gonna be at this location in memory.
// DON'T ADD new variables outside of the struct as it's likely to push our globals around.
// We NEED this shit to be in the same place so we don't have to change racman every single time lmao
typedef struct {
	// toggled via racman
	int   		modEnabled;    			// d9f000
	int   		saveCamPosition;   		// d9f004
	int   		loadCamPosition;		// d9f008
	int   		operatingCamera;		// d9f00c
	int   		lookAtMode;				// d9f010
	int   		lockWithoutStrafe;		// d9f014
	int   		isSmooth;				// d9f018
	
	// speed changes via racman
	float 		rotationSpeed;			// d9f01c
	float	 	moveSpeed;				// d9f020
	float 		rotationFriction; 		// d9f028
	float 		moveFriction; 			// d9f024
	
	// current save position... set via racman
	Savepos		savedPosition;			// d9f02c
	
	// DONT CARE.
	int   		init;
	int   		buttonCooldown;
	Vec4  		acceleration;
	Vec4  		velocity;
	Vec4  		lookatpos;
	Vec4  		lookAtTarget;
	Vec4  		worldUp;
	Vec4  		lock;
	Vec4   		iter;
	float 		yaw;
	float 		pitch;
	float 		yawVelocity;
	float 		pitchVelocity;
	float 		yawAcceleration;
	float 		pitchAcceleration;
	float 		horizontal;
	cellPadData prevInput;
} CAMERA_MOD;

// OUR STRUCT <3 
CAMERA_MOD c;

// Externs from symbols
extern int32_t cellPadGetData(uint32_t port_no, cellPadData *data);
extern void memset(void *ptr, int x, uint32_t n);
extern Camera* camera;
extern Vec4 player_coords;
extern int ui_toggle;
extern char occlusion[];

/////////////////////////////////////////////////////////////////////

void setlookAtTarget(){
	c.lookAtTarget.x = camera->pos.x + camera->forward.x;
	c.lookAtTarget.y = camera->pos.y + camera->forward.y;
	c.lookAtTarget.z = camera->pos.z + camera->forward.z;
	c.lookAtTarget.w = 0;
	vec_clear(&c.iter);	
}

void setLookAtRatchet(){
	c.lookAtTarget.x = player_coords.x;
	c.lookAtTarget.y = player_coords.y;
	c.lookAtTarget.z = player_coords.z;
	c.lookAtTarget.w = 0;
	vec_clear(&c.iter);
}

void lookAt(Vec4 pos){
	camera->forward.x = -(camera->pos.x - pos.x);
	camera->forward.y = -(camera->pos.y - pos.y);
	camera->forward.z = -(camera->pos.z - pos.z);
	camera->forward = normalize(camera->forward);
	Vec4 worldUp;
	worldUp.x = 0;
	worldUp.y = 0;
	worldUp.z = 1;
	worldUp.w = 0;		
	camera->right = normalize(cross(worldUp, camera->forward));
	camera->up = normalize(cross(camera->forward, camera->right));
	c.lookatpos = pos;
}

////////////////////////////////////////////////////////////////////

// KEEP FRICTION BETWEEN -1 <-> 0 OR YOU WILL ACCELERATE TOO FAST AND CRASH
//  -1 = MAXFRICTION, 0 = NO FRICTION;

////////////////////////////////////////////////////////////////////

int32_t pad_redirect(uint32_t port_no, cellPadData *data) {

	// Call to get the inputs from PS3 system
	int32_t ret = cellPadGetData(port_no, data);

	// Button cooldown
	if (c.buttonCooldown > 0)
		c.buttonCooldown--;
		
	// Toggles between normal gameplay and freecam. modEnabled is used in camlock.s to override the game's ability to move the camera.
	if(data->BTN_SELECT && c.buttonCooldown == 0){
		c.buttonCooldown = 20;
		c.operatingCamera = 1;
		c.lookAtMode = 0;
		c.modEnabled = !c.modEnabled;
	}
	// Make sure SELECT doesnt open the map, override the input completely.
	data->BTN_SELECT = NO_INPUT;

	// Toggle the UI if the mod is enabled or not.
	if(!c.modEnabled){
		ui_toggle = 0;
		return ret;
	}
	else {
		ui_toggle = 1;
	}
	
	// Check if length is 0, then we should use the inputs from the previous frame, which are stored at the bottom of the loop.
	// This ensures that our freecam doesn't move on it's own due to fake inputs.
	if (ret != 0 || data->len == 0)
		*data = c.prevInput;

	// Set occlusion on every frame that the stuff in the level doesn't randomly deload.
	memset(occlusion, 0xff, 0xff);

	// Initialize all variables if this is our first time running the freecam.
	if(!c.init){
		identity(c.acceleration);
		identity(c.velocity);
		identity(c.lookAtTarget);
		identity(c.lookatpos);
		c.iter.x = 0;
		c.iter.y = 0;
		c.iter.z = 0;
		c.iter.w = 0;
		c.worldUp.x = 0;
		c.worldUp.y = 0;
		c.worldUp.z = 1;
		c.worldUp.w = 0;
		c.rotationSpeed = 0.03f;
		c.moveSpeed = 0.05f;
		c.moveFriction = -0.1f;
		c.rotationFriction = -0.1f;
		c.lockWithoutStrafe = 0;
		c.horizontal = 0.0f;
		c.init = 1;
	}

	// Toggle for whether or not camera should move whilst in freecam mode.
	// Operating camera or operating ratchet
	if (data->BTN_START && c.buttonCooldown == 0){
		c.buttonCooldown = 20;
		c.operatingCamera = !c.operatingCamera;
	}
	
	// Cycle lookat mode.
	if (data->BTN_R2 && c.buttonCooldown == 0){
		c.buttonCooldown = 20;
		c.lookAtMode++;
		if (c.lookAtMode > 2)
			c.lookAtMode = 0;
		vec_clear(&c.iter);
	}
	
	// WORK IN PROGRESS. Smooth camera toggle
	/*
	if(data->BTN_CROSS && c.buttonCooldown == 0){
		c.buttonCooldown = 20;
		c.isSmooth = !c.isSmooth;
	}
	 */
	
	// Save our camera position to savedPosition if racman command was toggled
	if(c.saveCamPosition){
		c.savedPosition.cam = *camera;
		c.savedPosition.saved = 1;
		c.savedPosition.yaw = c.yaw;
		c.savedPosition.pitch = c.pitch;
		c.saveCamPosition = 0;
	}
	
	// Load our camera position, if one was already saved into savedPosition
	if(c.loadCamPosition){
		if (c.savedPosition.saved){
			*camera = c.savedPosition.cam;
			c.yaw = c.savedPosition.yaw;
			c.pitch = c.savedPosition.pitch;
			c.pitchVelocity = 0;
			c.yawVelocity = 0;
			vec_clear(&c.velocity);
			c.loadCamPosition = 0;
		}
	}
	
	// Constructing the camera axes
	float y = c.yaw * (PI / 180.0f);
	float p = c.pitch * (PI / 180.0f);
	
	float cp = cos(p);
	float sp = sin(p);
	float cy = cos(y);
	float sy = sin(y);
	
	camera->forward.x = cy * cp;
	camera->forward.y =	sy * cp;
	camera->forward.z =	-sp;
	
	camera->right.x = -sy;
	camera->right.y = cy;
	camera->right.z = 0.0f;
	
	camera->up.x = cy * sp;
	camera->up.y = sy * sp;
	camera->up.z = cp;
	
	
	// Whilst in normal freecam mode, pressing R3 sets a position to look at.
	if(data->BTN_R3 && !c.lookAtMode ){
		setlookAtTarget();
	}
	
	// Set look at mode. 
	switch(c.lookAtMode){
		case LOOK_AT_MODE_TARGET:
			if (c.lookAtTarget.x == 0.0f)
				setlookAtTarget(); 
			if (c.isSmooth){
				if((length(vec_add(c.lookAtTarget, scalar_multiplication(-1.0, c.lookatpos)))) - length(c.iter) < 0.01){
					lookAt(c.lookAtTarget);
				}
				else
					c.iter = vec_add(c.iter, scalar_multiplication(0.001f, vec_add(c.lookAtTarget, scalar_multiplication(-1.0, c.lookatpos))));
					lookAt(vec_add(c.iter, c.lookatpos)); 
			}
			else
				lookAt(c.lookAtTarget);
			break;
		case LOOK_AT_MODE_RATCHET:
			if (c.isSmooth){
				if((length(vec_add(player_coords, scalar_multiplication(-1.0, c.lookatpos)))) - length(c.iter) < 0.01){
					lookAt(player_coords);
				}
				else
					c.iter = vec_add(c.iter, scalar_multiplication(0.001f, vec_add(player_coords, scalar_multiplication(-1.0, c.lookatpos))));
					lookAt(vec_add(c.iter, c.lookatpos));
			}
			else
				lookAt(player_coords);
			break;
	}
	
	// Runs while we're able to freely operate the camera.
	if(c.operatingCamera){
		//RESET ACCELERATION SO IT DOESNT COMPOUND ENDLESSLY
		c.acceleration.x = 0;
		c.acceleration.y = 0;
		c.acceleration.z = 0;
		c.acceleration.w = 0;
		c.pitchAcceleration = 0;
		c.yawAcceleration = 0;
		
		//INTRODUCE FRICTION
		c.pitchVelocity = c.pitchVelocity + c.pitchVelocity * c.rotationFriction; 
		c.yawVelocity = c.yawVelocity + c.yawVelocity * c.rotationFriction;
		c.velocity = vec_add(c.velocity, scalar_multiplication(c.moveFriction, c.velocity));
		
		//Direction to move camera forward in the world regardless of camera pitch.
		Vec4 coolforwarddir;
		coolforwarddir = normalize(cross(camera->right, c.worldUp));
		
		
		//ADD TO ACCELERATION WITH INPUTS
		c.yawAcceleration += ((128 - data->ANA_R_H) / DEADTHRESH) * c.rotationSpeed;
		c.pitchAcceleration -= ((128 - data->ANA_R_V) / DEADTHRESH) * c.rotationSpeed;

		// Move with left stick
		if ((128 - data->ANA_L_H) > DEADTHRESH || (128 - data->ANA_L_H)  < -DEADTHRESH)
			c.acceleration = vec_add(c.acceleration, scalar_multiplication(((float)(128 - data->ANA_L_H)) * c.moveSpeed / 128.0f, camera->right));
		if ((128 - data->ANA_L_V) > DEADTHRESH || (128 - data->ANA_L_V)  < -DEADTHRESH)
			c.acceleration = vec_add(c.acceleration, scalar_multiplication(((float)(128 - data->ANA_L_V)) * c.moveSpeed / 128.0f, camera->forward));

		// Moves up or down.
		if (data->BTN_L1)
			c.acceleration.z += c.moveSpeed;
		if (data->BTN_L2)
			c.acceleration.z -= c.moveSpeed;

		// Move alongside X and Y planes in world space.
		if (data->BTN_UP)
			c.acceleration = vec_add(c.acceleration, scalar_multiplication(c.moveSpeed, coolforwarddir));
		if (data->BTN_DOWN)
			c.acceleration = vec_add(c.acceleration, scalar_multiplication(-c.moveSpeed, coolforwarddir));
		if (data->BTN_LEFT)
			c.acceleration = vec_add(c.acceleration, scalar_multiplication(c.moveSpeed, camera->right));
		if (data->BTN_RIGHT)
			c.acceleration = vec_add(c.acceleration, scalar_multiplication(-c.moveSpeed, camera->right));
		
		// Grab player where the freecam is moving. Like cam+char mode in debug menu
		if(data->BTN_R1 && !c.lookAtMode){
			player_coords = vec_add(camera->pos, scalar_multiplication(5.0f, camera->forward));
			data->BTN_L2 = 1;
		}	
		
		// INCREASE VELOCITY WITH ACCELERATION
		c.pitchVelocity = c.pitchVelocity + c.pitchAcceleration;
		c.yawVelocity = c.yawVelocity + c.yawAcceleration;
		c.velocity = vec_add(c.velocity, c.acceleration);
		
		// MOVE WITH VELOCITY
		c.yaw += c.yawVelocity;
		c.pitch += c.pitchVelocity;
		camera->pos = vec_add(camera->pos, c.velocity);
		
		// Limits for moving pitch with camera, limited 90deg up/down so you can't go upside down by accident
		if (c.pitch > TURN_RANGE)
			c.pitch = TURN_RANGE;
		else if (c.pitch < -TURN_RANGE)
			c.pitch = -TURN_RANGE;
	}

	if(!c.lockWithoutStrafe){
		// default
		c.lock = vec_add(scalar_multiplication(-1.0f, player_coords), camera->pos);
		c.horizontal = 0;
	}
	else {
		// lock without strafe.
		c.horizontal += ((128 - data->ANA_R_H) / DEADTHRESH) * 0.25f * PI / 180.0f;
		Vec4 temp;
		temp.x = cos(c.horizontal) * c.lock.x - sin(c.horizontal)*c.lock.y;
		temp.y = sin(c.horizontal) * c.lock.x + cos(c.horizontal)*c.lock.y;
		temp.z = c.lock.z;
		camera->pos = vec_add(player_coords, temp);
	}
	
	// Make sure these inputs don't actually go to the game while freecam mode is running.
	data->BTN_SELECT = NO_INPUT;
	data->BTN_START = NO_INPUT;
	data->BTN_R3 = NO_INPUT;
	data->BTN_UP = NO_INPUT;
	data->BTN_DOWN = NO_INPUT;
	data->BTN_LEFT = NO_INPUT;
	data->BTN_RIGHT = NO_INPUT;
	if(c.operatingCamera){
		data->BTN_L1 = NO_INPUT;
		data->ANA_L_H = NEUTRAL; 
		data->ANA_L_V = NEUTRAL;
	}
	
    data->ANA_R_H = NEUTRAL; 
    data->ANA_R_V = NEUTRAL;
	
	//¯\_( ͡° ͜ʖ ͡°)_/¯

	// Store current inputs for next frame
	c.prevInput = *data;
	return ret;
}

