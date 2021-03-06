/*==============================================================================
            Copyright (c) 2010-2012 QUALCOMM Austria Research Center GmbH.
            All Rights Reserved.
            Qualcomm Confidential and Proprietary
            
@file 
    FrameMarkers.cpp

@brief
    Sample for FrameMarkers

==============================================================================*/


#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <signal.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <QCAR/QCAR.h>
#include <QCAR/CameraDevice.h>
#include <QCAR/Renderer.h>
#include <QCAR/VideoBackgroundConfig.h>
#include <QCAR/Trackable.h>
#include <QCAR/TrackableResult.h>
#include <QCAR/MarkerResult.h>
#include <QCAR/Tool.h>
#include <QCAR/MarkerTracker.h>
#include <QCAR/TrackerManager.h>
#include <QCAR/CameraCalibration.h>
#include <QCAR/Marker.h>

#include "SampleMath.h"
#include "SampleUtils.h"
#include "Texture.h"
#include "CubeShaders.h"
#include "Q_object.h"
#include "C_object.h"
#include "A_object.h"
#include "R_object.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Textures:
int textureCount                = 0;
Texture** textures              = 0;

// OpenGL ES 2.0 specific:
unsigned int shaderProgramID    = 0;
GLint vertexHandle              = 0;
GLint normalHandle              = 0;
GLint textureCoordHandle        = 0;
GLint mvpMatrixHandle           = 0;
GLint texSampler2DHandle        = 0;

// Screen dimensions:
unsigned int screenWidth        = 0;
unsigned int screenHeight       = 0;

// Indicates whether screen is in portrait (true) or landscape (false) mode
bool isActivityInPortraitMode   = false;

// The projection matrix used for rendering virtual objects:
QCAR::Matrix44F projectionMatrix;

// Constants:
static const float kLetterScale        = 25.0f;
static const float kLetterTranslate    = 25.0f;

static struct sigaction old_sa[NSIG];
static JNIEnv *g_sigEnv;
static jobject g_sigObj;
static jmethodID g_sigNativeCrashed;

void android_sigaction(int signal, siginfo_t *info, void *reserved) {
    g_sigEnv->CallVoidMethod(g_sigObj, g_sigNativeCrashed);
    old_sa[signal].sa_handler(signal);
}

/*
JavaVM* gJavaVM = NULL;
jobject gJavaActivityClass;
const char* kJavActivityClassPath = "com/wheelphone/targetNavigation/FrameMarkersRenderer";
static JNINativeMethod methodTable[] = {
  {"Java_com_wheelphone_targetNavigation_FrameMarkersRenderer_renderFrame", "()V", (void *) Java_com_wheelphone_targetNavigation_FrameMarkersRenderer_renderFrame},
};
jint JNI_OnLoad(JavaVM* aVm, void* aReserved) {
	// cache java VM
	gJavaVM = aVm;

	JNIEnv* env;
	if (aVm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		LOGE("Failed to get the environment");
		return -1;
	}

	// Get SBCEngine activity class
	jclass activityClass = env->FindClass(kJavActivityClassPath);
	if (!activityClass) {
		LOGE("failed to get %s class reference", kJavActivityClassPath);
		return -1;
	}
	gJavaActivityClass = env->NewGlobalRef(activityClass);

	// Register methods with env->RegisterNatives.
	env->RegisterNatives(activityClass, methodTable, sizeof(methodTable) / sizeof(methodTable[0]));

	return JNI_VERSION_1_6;
}
*/

JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_setActivityPortraitMode(JNIEnv *, jobject, jboolean isPortrait)
{
    isActivityInPortraitMode = isPortrait;
}


JNIEXPORT int JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_initTracker(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_initTracker");
    
    // Initialize the marker tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* trackerBase = trackerManager.initTracker(QCAR::Tracker::MARKER_TRACKER);
    QCAR::MarkerTracker* markerTracker = static_cast<QCAR::MarkerTracker*>(trackerBase);
    if (markerTracker == NULL)
    {
        LOG("Failed to initialize MarkerTracker.");
        return 0;
    }
    
    if (!markerTracker->createFrameMarker(0, "MarkerQ", QCAR::Vec2F(50,50)))
    {
        LOG("Failed to create frame marker Q.");
    }
    if (!markerTracker->createFrameMarker(1, "MarkerC", QCAR::Vec2F(50,50)))
    {
        LOG("Failed to create frame marker C.");
    }
    if (!markerTracker->createFrameMarker(2, "MarkerA", QCAR::Vec2F(50,50)))
    {
        LOG("Failed to create frame marker A.");
    }
    if (!markerTracker->createFrameMarker(3, "MarkerR", QCAR::Vec2F(50,50)))
    {
        LOG("Failed to create frame marker R.");
    }
    
    LOG("Successfully initialized MarkerTracker.");

    return 1;
}


JNIEXPORT void JNICALL
    Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_deinitTracker(JNIEnv *, jobject)
{
    LOG("Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_deinitTracker");

    // Deinit the marker tracker, this will destroy all created frame markers:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    trackerManager.deinitTracker(QCAR::Tracker::MARKER_TRACKER);
}


JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_FrameMarkersRenderer_renderFrame(JNIEnv *env, jobject obj)
{

    //LOG("Java_com_wheelphone_targetNavigation_FrameMarkersRenderer_renderFrame");
 
    // Clear color and depth buffer 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get the state from QCAR and mark the beginning of a rendering section
    QCAR::State state = QCAR::Renderer::getInstance().begin();

    // Explicitly render the Video Background
    QCAR::Renderer::getInstance().drawVideoBackground();
    
 /*
    glEnable(GL_DEPTH_TEST);

    // We must detect if background reflection is active and adjust the culling direction. 
    // If the reflection is active, this means the post matrix has been reflected as well,
    // therefore standard counter clockwise face culling will result in "inside out" models. 
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    QCAR::VideoBackgroundConfig config = QCAR::Renderer::getInstance().getVideoBackgroundConfig();
    //if(config == NULL) {
    //	return;
    //}
    if(config.mReflection == QCAR::VIDEO_BACKGROUND_REFLECTION_ON) {
        glFrontFace(GL_CW);  //Front camera
        LOG("Using front camera");
    } else {
        glFrontFace(GL_CCW);   //Back camera
        LOG("Using back camera");
    }

    LOG("Num trackables = %d", state.getNumTrackableResults());

    // Did we find any trackables this frame?
    for(int tIdx = 0; tIdx < state.getNumTrackableResults(); tIdx++)
    {
    	LOG("tIdx = %d", tIdx);

        // Get the trackable:
        const QCAR::TrackableResult* trackableResult = state.getTrackableResult(tIdx);
        if(trackableResult == NULL) {
        	continue;
        }
        LOG("trackableResult status=%d, type=%d", trackableResult->getStatus(), trackableResult->getType());

        // get position and orientation of the target respect to the camera reference frame
        QCAR::Matrix44F modelViewMatrix =
            QCAR::Tool::convertPose2GLMatrix(trackableResult->getPose());
        //if(modelViewMatrix == NULL) {
        //	continue;
        //}

        // Choose the texture based on the target name:
        int textureIndex = 0;
        
        // Check the type of the trackable:
        assert(trackableResult->getType() == QCAR::TrackableResult::MARKER_RESULT);
        const QCAR::MarkerResult* markerResult = static_cast<
                                    const QCAR::MarkerResult*>(trackableResult);
        if(markerResult == NULL) {
        	continue;
        }
        const QCAR::Marker& marker = markerResult->getTrackable();
        //if(marker == NULL) {
        //	continue;
        //}

        textureIndex = marker.getMarkerId();
        LOG("textureIndex=%d", textureIndex);

	//markerSize = marker.getSize();
        
        assert(textureIndex < textureCount);
        const Texture* const thisTexture = textures[textureIndex];

        // Select which model to draw:
        const GLvoid* vertices = 0;
        const GLvoid* normals = 0;
        const GLvoid* indices = 0;
        const GLvoid* texCoords = 0;
        int numIndices = 0;	

        switch (textureIndex)
        {
        case 0:
            vertices = &QobjectVertices[0];
            normals = &QobjectNormals[0];
            indices = &QobjectIndices[0];
            texCoords = &QobjectTexCoords[0];
            numIndices = NUM_Q_OBJECT_INDEX;
            break;
        case 1:
            vertices = &CobjectVertices[0];
            normals = &CobjectNormals[0];
            indices = &CobjectIndices[0];
            texCoords = &CobjectTexCoords[0];
            numIndices = NUM_C_OBJECT_INDEX;            
            break;
        case 2:
            vertices = &AobjectVertices[0];
            normals = &AobjectNormals[0];
            indices = &AobjectIndices[0];
            texCoords = &AobjectTexCoords[0];
            numIndices = NUM_A_OBJECT_INDEX;
            break;
        default:
            vertices = &RobjectVertices[0];
            normals = &RobjectNormals[0];
            indices = &RobjectIndices[0];
            texCoords = &RobjectTexCoords[0];
            numIndices = NUM_R_OBJECT_INDEX;
            break;
        }

        QCAR::Matrix44F modelViewProjection;

        SampleUtils::translatePoseMatrix(-kLetterTranslate,
                                         -kLetterTranslate,
                                         0.f,
                                         &modelViewMatrix.data[0]);
        LOG("translatePoseMatrix called");

        SampleUtils::scalePoseMatrix(kLetterScale, kLetterScale, kLetterScale,
                                     &modelViewMatrix.data[0]);
        LOG("scalePoseMatrix called");

        SampleUtils::multiplyMatrix(&projectionMatrix.data[0],
                                    &modelViewMatrix.data[0],
                                    &modelViewProjection.data[0]);
        LOG("multiplyMatrix called");

        glUseProgram(shaderProgramID);
        LOG("glUseProgram called");
 
        glVertexAttribPointer(vertexHandle, 3, GL_FLOAT, GL_FALSE, 0, vertices);
        LOG("glVertexAttribPointer vertices called");
        glVertexAttribPointer(normalHandle, 3, GL_FLOAT, GL_FALSE, 0, normals);
        LOG("glVertexAttribPointer normals called");
        glVertexAttribPointer(textureCoordHandle, 2, GL_FLOAT, GL_FALSE,
                              0, texCoords);
        LOG("glVertexAttribPointer texcoords called");

        glEnableVertexAttribArray(vertexHandle);
        LOG("glEnableVertexAttribArray vertexHandle called");
        glEnableVertexAttribArray(normalHandle);
        LOG("glEnableVertexAttribArray normalHandle called");
        glEnableVertexAttribArray(textureCoordHandle);
        LOG("glEnableVertexAttribArray textureCoordHandle called");

        glActiveTexture(GL_TEXTURE0);
        LOG("glActiveTexture called");
        glBindTexture(GL_TEXTURE_2D, thisTexture->mTextureID);
        LOG("glBindTexture called");
        glUniformMatrix4fv(mvpMatrixHandle, 1, GL_FALSE,
                           (GLfloat*)&modelViewProjection.data[0]);
        LOG("glUniformMatrix4fv called");
        glUniform1i(texSampler2DHandle, 0); // GL_TEXTURE0
        LOG("glUniform1i called");
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, indices);
        LOG("glDrawElements called");

        SampleUtils::checkGlError("FrameMarkers render frame");
        LOG("checkGlError called");

    }

    glDisable(GL_DEPTH_TEST);
    glDisableVertexAttribArray(vertexHandle);
    glDisableVertexAttribArray(normalHandle);
    glDisableVertexAttribArray(textureCoordHandle);

    LOG("glDisables called");
*/
    QCAR::Renderer::getInstance().end();
}

JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_getTrackInfo(JNIEnv *env, jobject obj)
{

    // Get the state from QCAR and mark the beginning of a rendering section
    QCAR::State state = QCAR::Renderer::getInstance().begin();
    
	//QCAR::Vec2F markerSize;
   	jint jx[4] = {0};
	jint jy[4] = {0};            
	jfloat distance[4] = {0};
	jfloat cam_x[4] = {0}; 
	jfloat cam_y[4] = {0}; 
	jfloat cam_z[4] = {0}; 
	jfloat target_pose_x[4] = {0};	// x, y, z coordinates of the targets with respect to the camera frame
	jfloat target_pose_y[4] = {0};
	jfloat target_pose_z[4] = {0};
	
	jboolean detected[4] = {false};
	jclass javaClass = env->GetObjectClass(obj);	// obj is the java class object calling the "renderFrame" method, that is an FrameMarkersRenderer object
    //jclass javaClass = env->FindClass("Lcom/wheelphone/targetNavigation/WheelphoneTargetNavigation;"); // doesn't work!
	jmethodID method = env->GetMethodID(javaClass, "updateMarkersInfo", "(IZIIFFFF)V");
        
    // Did we find any trackables this frame?
    for(int tIdx = 0; tIdx < state.getNumTrackableResults(); tIdx++)
    {
        // Get the trackable:
        const QCAR::TrackableResult* trackableResult = state.getTrackableResult(tIdx);
        if(trackableResult == NULL) {
        	continue;
        }
        QCAR::Matrix44F modelViewMatrix =
            QCAR::Tool::convertPose2GLMatrix(trackableResult->getPose());        
        //if(modelViewMatrix == NULL) {
        //	continue;
        //}

        // Choose the texture based on the target name:
        int textureIndex = 0;
        
        // Check the type of the trackable:
        assert(trackableResult->getType() == QCAR::TrackableResult::MARKER_RESULT);
        const QCAR::MarkerResult* markerResult = static_cast<
                                    const QCAR::MarkerResult*>(trackableResult);
        if(markerResult == NULL) {
        	continue;
        }

        const QCAR::Marker& marker = markerResult->getTrackable();
        //if(marker == NULL) {
        //	continue;
        //}

        textureIndex = marker.getMarkerId();
        
		//markerSize = marker.getSize();	// this is the size specified during marker creation! Not the current size!

        assert(textureIndex < textureCount);

        // Select which model to draw:
        const GLvoid* vertices = 0;
        const GLvoid* normals = 0;
        const GLvoid* indices = 0;
        const GLvoid* texCoords = 0;
        int numIndices = 0;

		QCAR::Vec2F result(0,0);		     
		const QCAR::CameraCalibration& cameraCalibration = QCAR::CameraDevice::getInstance().getCameraCalibration();      
	    QCAR::Vec2F cameraPoint = QCAR::Tool::projectPoint(cameraCalibration, trackableResult->getPose(), QCAR::Vec3F(0, 0, 0));
	    QCAR::VideoMode videoMode = QCAR::CameraDevice::getInstance().getVideoMode(QCAR::CameraDevice::MODE_OPTIMIZE_QUALITY); //MODE_DEFAULT);
	    QCAR::VideoBackgroundConfig config = QCAR::Renderer::getInstance().getVideoBackgroundConfig();		
	    //if(config == NULL) {
	    //	continue;
	    //}
	    int xOffset = ((int) screenWidth - config.mSize.data[0]) / 2.0f + config.mPosition.data[0];
	    int yOffset = ((int) screenHeight - config.mSize.data[1]) / 2.0f - config.mPosition.data[1];
	
	    if (isActivityInPortraitMode)
	    {
	        // camera image is rotated 90 degrees
	        int rotatedX = videoMode.mHeight - cameraPoint.data[1];
	        int rotatedY = cameraPoint.data[0];
	
	        result = QCAR::Vec2F(rotatedX * config.mSize.data[0] / (float) videoMode.mHeight + xOffset,
	                           rotatedY * config.mSize.data[1] / (float) videoMode.mWidth + yOffset);
	    }
	    else
	    {
	        result = QCAR::Vec2F(cameraPoint.data[0] * config.mSize.data[0] / (float) videoMode.mWidth + xOffset,
	                           cameraPoint.data[1] * config.mSize.data[1] / (float) videoMode.mHeight + yOffset);
	    }		           
        jx[textureIndex] = (int)result.data[0];
        jy[textureIndex] = (int)result.data[1];
        // get position and orientation of the target respect to the camera reference frame
       	QCAR::Matrix34F pose = trackableResult->getPose();        
       	target_pose_x[textureIndex] = pose.data[3];
       	target_pose_y[textureIndex] = pose.data[7];
       	target_pose_z[textureIndex] = pose.data[11];
		QCAR::Vec3F position(pose.data[3], pose.data[7], pose.data[11]);
		// dist = modulo del vettore traslazione = sqrt(x*x + y*y + z*z)
		distance[textureIndex] = sqrt(position.data[0] * position.data[0] + position.data[1] * position.data[1] + position.data[2] * position.data[2]);            
        QCAR::Matrix44F inverseMV = SampleMath::Matrix44FInverse(modelViewMatrix);
        QCAR::Matrix44F invTranspMV = SampleMath::Matrix44FTranspose(inverseMV);
        // position of the camera and orientation axis with coordinates represented in the reference frame of the trackable
        //cam_x[textureIndex] = invTranspMV.data[4];
        //cam_y[textureIndex] = invTranspMV.data[5];
        cam_z[textureIndex] = invTranspMV.data[6]; 
        detected[textureIndex] = true;           	              

        //jstring js = env->NewStringUTF(marker.getName());
        //jclass javaClass = env->GetObjectClass(obj);
       	//jmethodID method = env->GetMethodID(javaClass, "displayMessage", "(Ljava/lang/String;)V");
       	//env->CallVoidMethod(obj, method, js);
    }
    
    // put outside the previous loop because we want to warn the java object of the state of the detection in all cases (both when detected and when not detected)
    for(int i=0; i<4; i++) {
    	// obj, method, marker id, detected, x screen coord, y screen coord, distance, robot orientation component, robot to target angle component y, robot to target angle component z);
		env->CallVoidMethod(obj, method, i, detected[i], jx[i], jy[i], distance[i], cam_z[i], target_pose_y[i], target_pose_z[i]);
	}	

    QCAR::Renderer::getInstance().end();
    
}

void
configureVideoBackground()
{
    // Get the default video mode:
    QCAR::CameraDevice& cameraDevice = QCAR::CameraDevice::getInstance();
    QCAR::VideoMode videoMode = cameraDevice.
                                getVideoMode(QCAR::CameraDevice::MODE_OPTIMIZE_QUALITY); //MODE_DEFAULT);

    // Configure the video background
    QCAR::VideoBackgroundConfig config;
    config.mEnabled = true;
    config.mSynchronous = true;
    config.mPosition.data[0] = 0.0f;
    config.mPosition.data[1] = 0.0f;
    
    if (isActivityInPortraitMode)
    {
        //LOG("configureVideoBackground PORTRAIT");
        config.mSize.data[0] = videoMode.mHeight
                                * (screenHeight / (float)videoMode.mWidth);
        config.mSize.data[1] = screenHeight;

        if(config.mSize.data[0] < screenWidth)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenWidth;
            config.mSize.data[1] = screenWidth * 
                              (videoMode.mWidth / (float)videoMode.mHeight);
        }
    }
    else
    {
        //LOG("configureVideoBackground LANDSCAPE");
        config.mSize.data[0] = screenWidth;
        config.mSize.data[1] = videoMode.mHeight
                            * (screenWidth / (float)videoMode.mWidth);

        if(config.mSize.data[1] < screenHeight)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenHeight
                                * (videoMode.mWidth / (float)videoMode.mHeight);
            config.mSize.data[1] = screenHeight;
        }
    }

    LOG("Configure Video Background : Video (%d,%d), Screen (%d,%d), mSize (%d,%d)", videoMode.mWidth, videoMode.mHeight, screenWidth, screenHeight, config.mSize.data[0], config.mSize.data[1]);

    // Set the config:
    QCAR::Renderer::getInstance().setVideoBackgroundConfig(config);
}


JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_initApplicationNative(
                            JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_initApplicationNative");
    
    g_sigEnv = env;
	jclass javaClass = env->GetObjectClass(obj);	// "obj" is the java class object calling the "initApplicationNative" method, that is an WheelphoneTargetNavigation instance.
    g_sigNativeCrashed = env->GetMethodID(javaClass, "onNativeCrashed", "()V");
    g_sigObj = env->NewGlobalRef(obj);
    struct sigaction handler;
    memset(&handler, 0, sizeof(struct sigaction));
    handler.sa_sigaction = android_sigaction;
    handler.sa_flags = SA_RESETHAND;
    #define CATCHSIG(X) sigaction(X, &handler, &old_sa[X])
    CATCHSIG(SIGILL);
    CATCHSIG(SIGABRT);
    CATCHSIG(SIGBUS);
    CATCHSIG(SIGFPE);
    CATCHSIG(SIGSEGV);
    CATCHSIG(SIGSTKFLT);
    CATCHSIG(SIGPIPE);

    // Store screen dimensions
    screenWidth = width;
    screenHeight = height;
        
    // Handle to the activity class:
    jclass activityClass = env->GetObjectClass(obj);

    jmethodID getTextureCountMethodID = env->GetMethodID(activityClass,
                                                    "getTextureCount", "()I");
    if (getTextureCountMethodID == 0)
    {
        LOG("Function getTextureCount() not found.");
        return;
    }

    textureCount = env->CallIntMethod(obj, getTextureCountMethodID);    
    if (!textureCount)
    {
       LOG("getTextureCount() returned zero.");
       return;
   }

   textures = new Texture*[textureCount];

   jmethodID getTextureMethodID = env->GetMethodID(activityClass,
       "getTexture", "(I)Lcom/wheelphone/targetNavigation/Texture;");

   if (getTextureMethodID == 0)
   {
       LOG("Function getTexture() not found.");
       return;
   }

   // Register the textures
   for (int i = 0; i < textureCount; ++i)
   {

       jobject textureObject = env->CallObjectMethod(obj, getTextureMethodID, i); 
       if (textureObject == NULL)
       {
           LOG("GetTexture() returned zero pointer");
           return;
       }

       textures[i] = Texture::create(env, textureObject);
   }

}


JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_deinitApplicationNative(
                                                        JNIEnv* env, jobject obj)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_deinitApplicationNative");

    // Uninstall the signal handlers.
	#define REMOVESIG(X) sigaction(X, &old_sa[X], NULL)
    REMOVESIG(SIGILL);
    REMOVESIG(SIGABRT);
    REMOVESIG(SIGBUS);
    REMOVESIG(SIGFPE);
    REMOVESIG(SIGSEGV);
    REMOVESIG(SIGSTKFLT);
    REMOVESIG(SIGPIPE);
    env->DeleteGlobalRef(g_sigObj);

    // Release texture resources
    if (textures != 0)
    {    
        for (int i = 0; i < textureCount; ++i)
        {
            delete textures[i];
            textures[i] = NULL;
        }
    
        delete[]textures;
        textures = NULL;
        
        textureCount = 0;
    }
}


JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_startCamera(JNIEnv *,
                                                                         jobject)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_startCamera");
    
    // Select the camera to open, set this to QCAR::CameraDevice::CAMERA_FRONT 
    // to activate the front camera instead.
    QCAR::CameraDevice::CAMERA camera = QCAR::CameraDevice::CAMERA_DEFAULT;

    // Initialize the camera:
    if (!QCAR::CameraDevice::getInstance().init(camera))
        return;

    // Select the default mode:
    // MODE_DEFAULT gives 480x320, MODE_OPTIMIZE_QUALITY gives 640x480 (sony xperia mini)	
    if (!QCAR::CameraDevice::getInstance().selectVideoMode(
                                QCAR::CameraDevice::MODE_OPTIMIZE_QUALITY)) //MODE_DEFAULT)) //
        return;

    // Configure the video background
    configureVideoBackground();

    // Select the default mode:
    //if (!QCAR::CameraDevice::getInstance().selectVideoMode(
    //                            QCAR::CameraDevice::MODE_DEFAULT))
    //    return;

    // Start the camera:
    if (!QCAR::CameraDevice::getInstance().start())
        return;

    // Start the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* markerTracker = trackerManager.getTracker(QCAR::Tracker::MARKER_TRACKER);
    if(markerTracker != 0)
        markerTracker->start();
    else 
	LOG("markerTracker not started!\n");
}


JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_stopCamera(JNIEnv *,
                                                                   jobject)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_stopCamera");
    
    // Stop the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* markerTracker = trackerManager.getTracker(QCAR::Tracker::MARKER_TRACKER);
    if(markerTracker != 0)
        markerTracker->stop();
    
    QCAR::CameraDevice::getInstance().stop();
    QCAR::CameraDevice::getInstance().deinit();
}

JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_setProjectionMatrix(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_setProjectionMatrix");

    // Cache the projection matrix:
    const QCAR::CameraCalibration& cameraCalibration =
                                QCAR::CameraDevice::getInstance().getCameraCalibration();
    projectionMatrix = QCAR::Tool::getProjectionGL(cameraCalibration, 2.0f, 2500.0f);
}


JNIEXPORT jboolean JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_autofocus(JNIEnv*, jobject)
{
    return QCAR::CameraDevice::getInstance().setFocusMode(QCAR::CameraDevice::FOCUS_MODE_TRIGGERAUTO) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_wheelphone_targetNavigation_WheelphoneTargetNavigation_setFocusMode(JNIEnv*, jobject, jint mode)
{
    int qcarFocusMode;

    switch ((int)mode)
    {
        case 0:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_NORMAL;
            break;
        
        case 1:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_CONTINUOUSAUTO;
            break;
            
        case 2:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_INFINITY;
            break;
            
        case 3:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_MACRO;
            break;
    
        default:
            return JNI_FALSE;
    }
    
    return QCAR::CameraDevice::getInstance().setFocusMode(qcarFocusMode) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_FrameMarkersRenderer_initRendering(
                                                    JNIEnv* env, jobject obj)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkersRenderer_initRendering");

    // Define clear color
    glClearColor(0.0f, 0.0f, 0.0f, QCAR::requiresAlpha() ? 0.0f : 1.0f);
    
    // Now generate the OpenGL texture objects and add settings
    for (int i = 0; i < textureCount; ++i)
    {
        glGenTextures(1, &(textures[i]->mTextureID));
        glBindTexture(GL_TEXTURE_2D, textures[i]->mTextureID);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[i]->mWidth,
                textures[i]->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                (GLvoid*)  textures[i]->mData);
    }
  
    shaderProgramID     = SampleUtils::createProgramFromBuffer(cubeMeshVertexShader,
                                                            cubeFragmentShader);

    vertexHandle        = glGetAttribLocation(shaderProgramID,
                                                "vertexPosition");
    normalHandle        = glGetAttribLocation(shaderProgramID,
                                                "vertexNormal");
    textureCoordHandle  = glGetAttribLocation(shaderProgramID,
                                                "vertexTexCoord");
    mvpMatrixHandle     = glGetUniformLocation(shaderProgramID,
                                                "modelViewProjectionMatrix");
    texSampler2DHandle  = glGetUniformLocation(shaderProgramID, 
                                                "texSampler2D");
}


JNIEXPORT void JNICALL
Java_com_wheelphone_targetNavigation_FrameMarkersRenderer_updateRendering(
                        JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkersRenderer_updateRendering");
    
    // Update screen dimensions
    screenWidth = width;
    screenHeight = height;

    // Reconfigure the video background
    configureVideoBackground();
}


#ifdef __cplusplus
}
#endif
