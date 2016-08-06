#include "HelloWorldScene.h"
#include "ui/CocosGUI.h"
#include "cocostudio/CocoStudio.h"
#include "spine/spine-cocos2dx.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    auto layer = HelloWorld::create();
    
    // add layer as a child to scene
    scene->addChild(layer);
    
    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Layer::init() )
    {
        return false;
    }
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.
    
    // add a "close" icon to exit the progress. it's an autorelease object
    auto closeItem = MenuItemImage::create(
                                           "CloseNormal.png",
                                           "CloseSelected.png",
                                           CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));
    
    closeItem->setPosition(Vec2(origin.x + visibleSize.width - closeItem->getContentSize().width/2 ,
                                origin.y + closeItem->getContentSize().height/2));
    
    // create menu, it's an autorelease object
    auto menu = Menu::create(closeItem, NULL);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 1);
    
    /////////////////////////////
    // 3. add your codes below...
    
    // add a label shows "Hello World"
    // create and initialize a label
    
//        cocos2d::Node* pNode = cocos2d::CSLoader::createNode("Login.csb");
//        this->addChild(pNode);
    
    //    m_root = dynamic_cast<cocos2d::ui::Layout* >(pNode->getChildByName("panel_root"));
    //
    
    cocos2d::LayerColor* pColorLayer = cocos2d::LayerColor::create(cocos2d::Color4B::WHITE, visibleSize.width, visibleSize.height);
    this->addChild(pColorLayer);
    
    spine::SkeletonAnimation* pAnimation = spine::SkeletonAnimation::createWithFile("spine/Arena.json", "spine/Arena.atlas");
    pAnimation->setAnimation(0, "idle", true);
    pAnimation->setPosition(cocos2d::Vec2(visibleSize.width/2, visibleSize.height/2));
    this->addChild(pAnimation);
    
    return true;
}


void HelloWorld::menuCloseCallback(Ref* pSender)
{

}
