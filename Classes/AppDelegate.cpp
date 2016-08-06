#include "AppDelegate.h"
#include "HelloWorldScene.h"
#include "ShaderManager.hpp"

USING_NS_CC;

static cocos2d::Size designResolutionSize = cocos2d::Size(640, 960);

AppDelegate::AppDelegate() {

}

AppDelegate::~AppDelegate() 
{
}

//if you want a different context,just modify the value of glContextAttrs
//it will takes effect on all platforms
void AppDelegate::initGLContextAttrs()
{
    //set OpenGL context attributions,now can only set six attributions:
    //red,green,blue,alpha,depth,stencil
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8};

    GLView::setGLContextAttrs(glContextAttrs);
}

// If you want to use packages manager to install more packages, 
// don't modify or remove this function
static int register_all_packages()
{
    return 0; //flag for packages manager
}

bool AppDelegate::applicationDidFinishLaunching() {
    // initialize director
    auto director = Director::getInstance();
    auto glview = director->getOpenGLView();
    if(!glview) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
        glview = GLViewImpl::createWithRect("ProjectSLG", Rect(0, 0, designResolutionSize.width, designResolutionSize.height));
#else
        glview = GLViewImpl::create("ProjectSLG");
#endif
        director->setOpenGLView(glview);
    }

    // turn on display FPS
    #if COCOS2D_DEBUG == 1
    director->setDisplayStats(true);
    #endif

    // set FPS. the default value is 1.0/60 if you don't call this
    
    director->setAnimationInterval(1.0 / 60);
    // Set the design resolution
    glview->setDesignResolutionSize(designResolutionSize.width, designResolutionSize.height, ResolutionPolicy::FIXED_WIDTH);
    Size frameSize = glview->getFrameSize();
    
    // if the frame's width is smaller than the width of design size.
    if(frameSize.width < designResolutionSize.width)
    {
        director->setContentScaleFactor(MIN(frameSize.width / designResolutionSize.width, frameSize.height / designResolutionSize.height));
    }
    
    register_all_packages();

    FileUtils::getInstance()->addSearchPath("res");
    
    ShaderManager::getInstance()->loadGLPrograms();
    
    // create a scene. it's an autorelease object
    auto scene = Scene::create();
    auto pLayer = HelloWorld::create();
    scene->addChild(pLayer);
    // run
    cocos2d::Director::getInstance()->runWithScene(scene);
    return true;
}

// This function will be called when the app is inactive. When comes a phone call,it's be invoked too
void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();
    // if you use SimpleAudioEngine, it must be pause
    // SimpleAudioEngine::getInstance()->pauseBackgroundMusic();
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();
    // if you use SimpleAudioEngine, it must resume here
    // SimpleAudioEngine::getInstance()->resumeBackgroundMusic();
}
