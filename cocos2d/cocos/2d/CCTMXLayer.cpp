/****************************************************************************
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2014 Chukong Technologies Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "2d/CCTMXLayer.h"
#include "2d/CCTMXTiledMap.h"
#include "2d/CCSprite.h"
#include "base/CCDirector.h"
#include "renderer/CCTextureCache.h"
#include "renderer/CCGLProgram.h"
#include "deprecated/CCString.h" // For StringUtils::format

NS_CC_BEGIN


// TMXLayer - init & alloc & dealloc
// TMXLayer - atlasIndex and Z
static inline int compareInts(const void * a, const void * b)
{
    return ((*(int*)a) - (*(int*)b));
}

TMXLayer * TMXLayer::create(TMXTilesetInfo *tilesetInfo, TMXLayerInfo *layerInfo, TMXMapInfo *mapInfo)
{
    TMXLayer *ret = new (std::nothrow) TMXLayer();
    if (ret->initWithTilesetInfo(tilesetInfo, layerInfo, mapInfo))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
bool TMXLayer::initWithTilesetInfo(TMXTilesetInfo *tilesetInfo, TMXLayerInfo *layerInfo, TMXMapInfo *mapInfo)
{    
    // FIXME:: is 35% a good estimate ?
    Size size = layerInfo->_layerSize;
    
    Size winSize = Director::getInstance()->getWinSize();
    Size tileSize = mapInfo->getTileSize();
    // display tiles totle number
    float totalNumberOfTiles = powf(MAX(ceilf(winSize.width / tileSize.width), ceilf(winSize.height / tileSize.height)) * 4, 2);
    float capacity = totalNumberOfTiles;

    Texture2D *texture = nullptr;
    if( tilesetInfo )
    {
        texture = Director::getInstance()->getTextureCache()->addImage(tilesetInfo->_sourceImage.c_str());
    }

    if (nullptr == texture)
        return false;

    if (SpriteBatchNode::initWithTexture(texture, static_cast<ssize_t>(capacity)))
    {
        // layerInfo
        _layerName = layerInfo->_name;
        _layerSize = size;
        _tiles = layerInfo->_tiles;
        _opacity = layerInfo->_opacity;
        setProperties(layerInfo->getProperties());
        _contentScaleFactor = Director::getInstance()->getContentScaleFactor(); 

        // tilesetInfo
        _tileSet = tilesetInfo;
        CC_SAFE_RETAIN(_tileSet);

        // mapInfo
        _mapTileSize = mapInfo->getTileSize();
        _layerOrientation = mapInfo->getOrientation();

        // offset (after layer orientation is set);
        Vec2 offset = this->calculateLayerOffset(layerInfo->_offset);
        this->setPosition(CC_POINT_PIXELS_TO_POINTS(offset));

        _atlasIndexArray = ccCArrayNew(totalNumberOfTiles);

        this->setContentSize(CC_SIZE_PIXELS_TO_POINTS(Size(_layerSize.width * _mapTileSize.width, _layerSize.height * _mapTileSize.height)));

        _useAutomaticVertexZ = false;
        _vertexZvalue = 0;
        
        return true;
    }
    return false;
}

TMXLayer::TMXLayer()
:_layerName("")
,_opacity(0)
,_vertexZvalue(0)
,_useAutomaticVertexZ(false)
,_reusedTile(nullptr)
,_atlasIndexArray(nullptr)
,_contentScaleFactor(1.0f)
,_layerSize(Size::ZERO)
,_mapTileSize(Size::ZERO)
,_tiles(nullptr)
,_tileSet(nullptr)
,_layerOrientation(TMXOrientationOrtho)
,_currentDisplayTilePosition(Vec2::ZERO)
{}

TMXLayer::~TMXLayer()
{
    CC_SAFE_RELEASE(_tileSet);
    CC_SAFE_RELEASE(_reusedTile);

    if (_atlasIndexArray)
    {
        ccCArrayFree(_atlasIndexArray);
        _atlasIndexArray = nullptr;
    }

    CC_SAFE_DELETE_ARRAY(_tiles);
}

void TMXLayer::releaseMap()
{
    if (_tiles)
    {
        delete [] _tiles;
        _tiles = nullptr;
    }

    if (_atlasIndexArray)
    {
        ccCArrayFree(_atlasIndexArray);
        _atlasIndexArray = nullptr;
    }
}

// TMXLayer - setup Tiles
void TMXLayer::setupTiles()
{    
    // Optimization: quick hack that sets the image size on the tileset
    _tileSet->_imageSize = _textureAtlas->getTexture()->getContentSizeInPixels();

    // By default all the tiles are aliased
    // pros:
    //  - easier to render
    // cons:
    //  - difficult to scale / rotate / etc.
    _textureAtlas->getTexture()->setAliasTexParameters();

    //CFByteOrder o = CFByteOrderGetCurrent();

    // Parse cocos2d properties
    this->parseInternalProperties();

    Size winSize = Director::getInstance()->getWinSize();
    
    Vec2 tile = getTilePositionAt(getPosition());

    int maxY = tile.y + ceilf(winSize.width / _mapTileSize.width) + 6;
    int minY = tile.y - (ceilf(winSize.height / _mapTileSize.height) + 5);
    int maxX = tile.x + ceilf(winSize.width / _mapTileSize.width) + 6;
    int minX = tile.x - (ceilf(winSize.height / _mapTileSize.height) + 5);

    for (int y=MAX(0, minY); y < MIN(_layerSize.height, maxY); y++)
    {
        for (int x=MAX(0, minX); x < MIN(_layerSize.width, maxX); x++)
        {
            int pos = static_cast<int>(x + _layerSize.width * y);
            int gid = _tiles[ pos ];

            // gid are stored in little endian.
            // if host is big endian, then swap
            //if( o == CFByteOrderBigEndian )
            //    gid = CFSwapInt32( gid );
            /* We support little endian.*/

            // FIXME:: gid == 0 --> empty tile
            if (gid != 0) 
            {
                this->appendTileForGID(gid, Vec2(x, y));
            }
        }
    }
}

// TMXLayer - Properties
Value TMXLayer::getProperty(const std::string& propertyName) const
{
    if (_properties.find(propertyName) != _properties.end())
        return _properties.at(propertyName);
    
    return Value();
}

void TMXLayer::parseInternalProperties()
{
    // if cc_vertex=automatic, then tiles will be rendered using vertexz

    auto vertexz = getProperty("cc_vertexz");
    if (!vertexz.isNull())
    {
        std::string vertexZStr = vertexz.asString();
        // If "automatic" is on, then parse the "cc_alpha_func" too
        if (vertexZStr == "automatic")
        {
            _useAutomaticVertexZ = true;
            auto alphaFuncVal = getProperty("cc_alpha_func");
            float alphaFuncValue = alphaFuncVal.asFloat();
            setGLProgramState(GLProgramState::getOrCreateWithGLProgramName(GLProgram::SHADER_NAME_POSITION_TEXTURE_ALPHA_TEST));

            GLint alphaValueLocation = glGetUniformLocation(getGLProgram()->getProgram(), GLProgram::UNIFORM_NAME_ALPHA_TEST_VALUE);

            // NOTE: alpha test shader is hard-coded to use the equivalent of a glAlphaFunc(GL_GREATER) comparison
            
            // use shader program to set uniform
            getGLProgram()->use();
            getGLProgram()->setUniformLocationWith1f(alphaValueLocation, alphaFuncValue);
            CHECK_GL_ERROR_DEBUG();
        }
        else
        {
            _vertexZvalue = vertexz.asInt();
        }
    }
}

void TMXLayer::setupTileSprite(Sprite* sprite, Vec2 pos, int gid)
{
    sprite->setPosition(getPositionAt(pos));
    sprite->setPositionZ((float)getVertexZForPos(pos));
    sprite->setAnchorPoint(Vec2::ZERO);
    sprite->setOpacity(_opacity);

    //issue 1264, flip can be undone as well
    sprite->setFlippedX(false);
    sprite->setFlippedY(false);
    sprite->setRotation(0.0f);
    sprite->setAnchorPoint(Vec2(0,0));

    // Rotation in tiled is achieved using 3 flipped states, flipping across the horizontal, vertical, and diagonal axes of the tiles.
    if (gid & kTMXTileDiagonalFlag)
    {
        // put the anchor in the middle for ease of rotation.
        sprite->setAnchorPoint(Vec2(0.5f,0.5f));
        sprite->setPosition(getPositionAt(pos).x + sprite->getContentSize().height/2,
           getPositionAt(pos).y + sprite->getContentSize().width/2 );

        int flag = gid & (kTMXTileHorizontalFlag | kTMXTileVerticalFlag );

        // handle the 4 diagonally flipped states.
        if (flag == kTMXTileHorizontalFlag)
        {
            sprite->setRotation(90.0f);
        }
        else if (flag == kTMXTileVerticalFlag)
        {
            sprite->setRotation(270.0f);
        }
        else if (flag == (kTMXTileVerticalFlag | kTMXTileHorizontalFlag) )
        {
            sprite->setRotation(90.0f);
            sprite->setFlippedX(true);
        }
        else
        {
            sprite->setRotation(270.0f);
            sprite->setFlippedX(true);
        }
    }
    else
    {
        if (gid & kTMXTileHorizontalFlag)
        {
            sprite->setFlippedX(true);
        }

        if (gid & kTMXTileVerticalFlag)
        {
            sprite->setFlippedY(true);
        }
    }
}

Sprite* TMXLayer::reusedTileWithRect(Rect rect)
{
    if (! _reusedTile) 
    {
        _reusedTile = Sprite::createWithTexture(_textureAtlas->getTexture(), rect);
        _reusedTile->setBatchNode(this);
        _reusedTile->retain();
    }
    else
    {
        // FIXME: HACK: Needed because if "batch node" is nil,
        // then the Sprite'squad will be reset
        _reusedTile->setBatchNode(nullptr);
        
        // Re-init the sprite
        _reusedTile->setTextureRect(rect, false, rect.size);
        
        // restore the batch node
        _reusedTile->setBatchNode(this);
    }

    return _reusedTile;
}

// TMXLayer - obtaining tiles/gids
bool TMXLayer::getTileAt(const Vec2& pos)
{
    CCASSERT(pos.x < _layerSize.width && pos.y < _layerSize.height && pos.x >=0 && pos.y >=0, "TMXLayer: invalid position");
    CCASSERT(_tiles && _atlasIndexArray, "TMXLayer: the tiles map has been released");

    Sprite *tile = nullptr;
    int gid = this->getTileGIDAt(pos);

    // if GID == 0, then no tile is present
    if (gid) 
    {
        int z = (int)(pos.x + pos.y * _layerSize.width);
        //tile = static_cast<Sprite*>(this->getChildByTag(z));

        // tile not created yet. create it
//        if (! tile) 
//        {
//            Rect rect = _tileSet->getRectForGID(gid);
//            rect = CC_RECT_PIXELS_TO_POINTS(rect);
//
//            tile = Sprite::createWithTexture(this->getTexture(), rect);
//            tile->setBatchNode(this);
//            tile->setPosition(getPositionAt(pos));
//            tile->setPositionZ((float)getVertexZForPos(pos));
//            tile->setAnchorPoint(Vec2::ZERO);
//            tile->setOpacity(_opacity);
//
//            ssize_t indexForZ = atlasIndexForExistantZ(z);
//            this->addSpriteWithoutQuad(tile, static_cast<int>(indexForZ), z);
//        }
        int *item = (int*)bsearch((void*)&z, (void*)&_atlasIndexArray->arr[0], _atlasIndexArray->num, sizeof(void*), compareInts);

        return item != nullptr;
    }
    
    return tile;
}

uint32_t TMXLayer::getTileGIDAt(const Vec2& pos, TMXTileFlags* flags/* = nullptr*/)
{
    CCASSERT(pos.x < _layerSize.width && pos.y < _layerSize.height && pos.x >=0 && pos.y >=0, "TMXLayer: invalid position");
    CCASSERT(_tiles && _atlasIndexArray, "TMXLayer: the tiles map has been released");

    ssize_t idx = static_cast<int>(((int) pos.x + (int) pos.y * _layerSize.width));
    // Bits on the far end of the 32-bit global tile ID are used for tile flags
    uint32_t tile = _tiles[idx];

    // issue1264, flipped tiles can be changed dynamically
    if (flags) 
    {
        *flags = (TMXTileFlags)(tile & kTMXFlipedAll);
    }
    
    return (tile & kTMXFlippedMask);
}

// TMXLayer - adding helper methods
Sprite * TMXLayer::insertTileForGID(uint32_t gid, const Vec2& pos)
{
    if (gid != 0 && (static_cast<int>((gid & kTMXFlippedMask)) - _tileSet->_firstGid) >= 0)
    {
        Rect rect = _tileSet->getRectForGID(gid);
        rect = CC_RECT_PIXELS_TO_POINTS(rect);
        
        intptr_t z = (intptr_t)((int) pos.x + (int) pos.y * _layerSize.width);
        
        Sprite *tile = reusedTileWithRect(rect);
        
        setupTileSprite(tile, pos, gid);
        
        // get atlas index
        ssize_t indexForZ = atlasIndexForNewZ(static_cast<int>(z));
        
        // Optimization: add the quad without adding a child
        this->insertQuadFromSprite(tile, indexForZ);
        
        // insert it into the local atlasindex array
        ccCArrayInsertValueAtIndex(_atlasIndexArray, (void*)z, indexForZ);
        
        // update possible children
        
        for(const auto &child : _children) {
            Sprite* sp = static_cast<Sprite*>(child);
            ssize_t ai = sp->getAtlasIndex();
            if ( ai >= indexForZ )
            {
                sp->setAtlasIndex(ai+1);
            }
        }
        
        _tiles[z] = gid;
        
        return tile;
    }
    
    return nullptr;
}

Sprite * TMXLayer::updateTileForGID(uint32_t gid, const Vec2& pos)
{
    Rect rect = _tileSet->getRectForGID(gid);
    rect = Rect(rect.origin.x / _contentScaleFactor, rect.origin.y / _contentScaleFactor, rect.size.width/ _contentScaleFactor, rect.size.height/ _contentScaleFactor);
    int z = (int)((int) pos.x + (int) pos.y * _layerSize.width);

    Sprite *tile = reusedTileWithRect(rect);

    setupTileSprite(tile ,pos ,gid);

    // get atlas index
    ssize_t indexForZ = atlasIndexForExistantZ(z);
    tile->setAtlasIndex(indexForZ);
    tile->setDirty(true);
    tile->updateTransform();
    _tiles[z] = gid;

    return tile;
}

// used only when parsing the map. useless after the map was parsed
// since lot's of assumptions are no longer true
Sprite * TMXLayer::appendTileForGID(uint32_t gid, const Vec2& pos)
{
    if (gid != 0 && (static_cast<int>((gid & kTMXFlippedMask)) - _tileSet->_firstGid) >= 0)
    {
        Rect rect = _tileSet->getRectForGID(gid);
        rect = CC_RECT_PIXELS_TO_POINTS(rect);
        
        intptr_t z = (intptr_t)(pos.x + pos.y * _layerSize.width);
        
        Sprite *tile = reusedTileWithRect(rect);
        
        setupTileSprite(tile ,pos ,gid);
        
        // optimization:
        // The difference between appendTileForGID and insertTileforGID is that append is faster, since
        // it appends the tile at the end of the texture atlas
        ssize_t indexForZ = _atlasIndexArray->num;
        
        // don't add it using the "standard" way.
        insertQuadFromSprite(tile, indexForZ);
        
        // append should be after addQuadFromSprite since it modifies the quantity values
        ccCArrayInsertValueAtIndex(_atlasIndexArray, (void*)z, indexForZ);
        
        _tilePositions.insert(std::pair<int, Vec2>(z, pos));

        return tile;
    }
    
    return nullptr;
}


ssize_t TMXLayer::atlasIndexForExistantZ(int z)
{
    int key=z;
    int *item = (int*)bsearch((void*)&key, (void*)&_atlasIndexArray->arr[0], _atlasIndexArray->num, sizeof(void*), compareInts);

    CCASSERT(item, "TMX atlas index not found. Shall not happen");

    ssize_t index = ((size_t)item - (size_t)_atlasIndexArray->arr) / sizeof(void*);
    return index;
}

ssize_t TMXLayer::atlasIndexForNewZ(int z)
{
    // FIXME:: This can be improved with a sort of binary search
    ssize_t i=0;
    for (i=0; i< _atlasIndexArray->num ; i++) 
    {
        ssize_t val = (size_t) _atlasIndexArray->arr[i];
        if (z < val)
        {
            break;
        }
    } 
    
    return i;
}

// TMXLayer - adding / remove tiles
void TMXLayer::setTileGID(uint32_t gid, const Vec2& pos)
{
    setTileGID(gid, pos, (TMXTileFlags)0);
}

void TMXLayer::setTileGID(uint32_t gid, const Vec2& pos, TMXTileFlags flags)
{
    CCASSERT(pos.x < _layerSize.width && pos.y < _layerSize.height && pos.x >=0 && pos.y >=0, "TMXLayer: invalid position");
    CCASSERT(_tiles && _atlasIndexArray, "TMXLayer: the tiles map has been released");
    CCASSERT(gid == 0 || (int)gid >= _tileSet->_firstGid, "TMXLayer: invalid gid" );

    TMXTileFlags currentFlags;
    uint32_t currentGID = getTileGIDAt(pos, &currentFlags);

    if (currentGID != gid || currentFlags != flags) 
    {
        uint32_t gidAndFlags = gid | flags;

        // setting gid=0 is equal to remove the tile
        if (gid == 0)
        {
            removeTileAt(pos);
        }
        // empty tile. create a new one
        else if (currentGID == 0)
        {
            insertTileForGID(gidAndFlags, pos);
            int z = (int) pos.x + (int) pos.y * _layerSize.width;
            _tilePositions.insert(std::pair<int, Vec2>(z, pos));
        }
        // modifying an existing tile with a non-empty tile
        else 
        {
            int z = (int) pos.x + (int) pos.y * _layerSize.width;
            Sprite *sprite = static_cast<Sprite*>(getChildByTag(z));
            if (sprite)
            {
                Rect rect = _tileSet->getRectForGID(gid);
                rect = CC_RECT_PIXELS_TO_POINTS(rect);

                sprite->setTextureRect(rect, false, rect.size);
                if (flags) 
                {
                    setupTileSprite(sprite, sprite->getPosition(), gidAndFlags);
                }
                _tiles[z] = gidAndFlags;
            } 
            else 
            {
                int *item = (int*)bsearch((void*)&z, (void*)&_atlasIndexArray->arr[0], _atlasIndexArray->num, sizeof(void*), compareInts);

                if(item)
                    updateTileForGID(gidAndFlags, pos);
                else
                    insertTileForGID(gidAndFlags, pos); 
            }
            
            _tilePositions.insert(std::pair<int, Vec2>(z, pos));
        }
    }
}

void TMXLayer::addTile(const Vec2& positon, const Size& size)
{
    Vec2 displayTile = this->getTilePositionAt(positon);

    displayTile.x = MIN(this->getLayerSize().width, displayTile.x);
    displayTile.y = MIN(this->getLayerSize().height, displayTile.y);
    
    if((std::abs(_currentDisplayTilePosition.x - displayTile.x) < 0.1) && (std::abs(_currentDisplayTilePosition.y - displayTile.y) < 0.1))
        return;
    
    _currentDisplayTilePosition.x = displayTile.x;
    _currentDisplayTilePosition.y = displayTile.y;
    
    int maxY = MIN(this->getLayerSize().height, displayTile.y + ceilf(size.width / this->getMapTileSize().width) + 6);
    int minY = MAX(0, displayTile.y - (ceilf(size.height / this->getMapTileSize().height) + 5));
    int maxX = MIN(this->getLayerSize().width, displayTile.x + ceilf(size.width / this->getMapTileSize().width) + 6);
    int minX = MAX(0, displayTile.x - (ceilf(size.height / this->getMapTileSize().height) + 5));
    
    _cuurentDisplayRect.origin.x = minX;
    _cuurentDisplayRect.origin.y = minY;
    _cuurentDisplayRect.size.width = MAX(0, maxX- minX);
    _cuurentDisplayRect.size.height = MAX(0, maxY - minY);
    
    std::vector<Vec2> displayTilePositions;
    for (int y = minY; y < maxY; y++)
    {
        for (int x = minX; x < maxX; x++)
        {
            displayTilePositions.push_back(Vec2(x, y));
                                           
            int z = static_cast<int>(x + _layerSize.width * y);

            auto it = _tilePositions.find(z);
            
            if(it != _tilePositions.end())
            {
                _tilePositions.erase(it);
                continue;
            }
            
            Vec2 tile(x, y);
            this->insertTileForGID(this->getTileGIDAt(tile), tile);
        }
    }
    
    
    if(!_tilePositions.empty())
    {
        auto it = _tilePositions.begin();
        
        while(it != _tilePositions.end())
        {
            Vec2& tilePosition = it->second;
            
            removeTileNoDeleteGID(tilePosition);
            if(m_removeTileCallback)
            {
                m_removeTileCallback(this, tilePosition);
            }
            ++it;
        }
        
        _tilePositions.clear();
    }
    
    if(!displayTilePositions.empty())
    {
        auto it = displayTilePositions.begin();
        
        while(it != displayTilePositions.end())
        {
            Vec2& tilePosition = *it;
            
            int z = static_cast<int>(tilePosition.x + _layerSize.width * tilePosition.y);
            
            _tilePositions.insert(std::pair<int, Vec2>(z, tilePosition));
            
            ++it;
        }
    }
}

void TMXLayer::addChild(Node * child, int zOrder, int tag)
{
    CC_UNUSED_PARAM(child);
    CC_UNUSED_PARAM(zOrder);
    CC_UNUSED_PARAM(tag);
    CCASSERT(0, "addChild: is not supported on TMXLayer. Instead use setTileGID:at:/tileAt:");
}

void TMXLayer::removeChild(Node* node, bool cleanup)
{
    Sprite *sprite = (Sprite*)node;
    // allows removing nil objects
    if (! sprite)
    {
        return;
    }

    CCASSERT(_children.contains(sprite), "Tile does not belong to TMXLayer");

    ssize_t atlasIndex = sprite->getAtlasIndex();
    ssize_t zz = (ssize_t)_atlasIndexArray->arr[atlasIndex];
    _tiles[zz] = 0;
    ccCArrayRemoveValueAtIndex(_atlasIndexArray, atlasIndex);
    SpriteBatchNode::removeChild(sprite, cleanup);
}

void TMXLayer::removeTileAt(const Vec2& pos)
{
    CCASSERT(pos.x < _layerSize.width && pos.y < _layerSize.height && pos.x >=0 && pos.y >=0, "TMXLayer: invalid position");
    CCASSERT(_tiles && _atlasIndexArray, "TMXLayer: the tiles map has been released");

    int gid = getTileGIDAt(pos);

    if (gid) 
    {
        int z = pos.x + pos.y * _layerSize.width;
        ssize_t atlasIndex = atlasIndexForExistantZ(z);

        // remove tile from GID map
        _tiles[z] = 0;

        // remove tile from atlas position array
        ccCArrayRemoveValueAtIndex(_atlasIndexArray, atlasIndex);

        // remove it from sprites and/or texture atlas
        Sprite *sprite = (Sprite*)getChildByTag(z);
        if (sprite)
        {
            SpriteBatchNode::removeChild(sprite, true);
        }
        else 
        {
            _textureAtlas->removeQuadAtIndex(atlasIndex);

            // update possible children
            for(const auto &obj : _children) {
                Sprite* child = static_cast<Sprite*>(obj);
                ssize_t ai = child->getAtlasIndex();
                if ( ai >= atlasIndex )
                {
                    child->setAtlasIndex(ai-1);
                }
            }
        }
    }
}

void TMXLayer::removeTileNoDeleteGID(const cocos2d::Vec2 &tileCoordinate)
{
    CCASSERT(tileCoordinate.x < _layerSize.width && tileCoordinate.y < _layerSize.height && tileCoordinate.x >=0 && tileCoordinate.y >=0,
             "TMXLayer: invalid position");
    CCASSERT(_tiles && _atlasIndexArray, "TMXLayer: the tiles map has been released");
    
    int gid = getTileGIDAt(tileCoordinate);
    
    if (gid)
    {
        int z = tileCoordinate.x + tileCoordinate.y * _layerSize.width;
        ssize_t atlasIndex = atlasIndexForExistantZ(z);
        
        // remove tile from atlas position array
        ccCArrayRemoveValueAtIndex(_atlasIndexArray, atlasIndex);
        
        // remove it from sprites and/or texture atlas
        Sprite *sprite = (Sprite*)getChildByTag(z);
        if (sprite)
        {
            SpriteBatchNode::removeChild(sprite, true);
        }
        else
        {
            _textureAtlas->removeQuadAtIndex(atlasIndex);
            
            // update possible children
            for(const auto &obj : _children) {
                Sprite* child = static_cast<Sprite*>(obj);
                ssize_t ai = child->getAtlasIndex();
                if ( ai >= atlasIndex )
                {
                    child->setAtlasIndex(ai-1);
                }
            }
        }
    }
}

void TMXLayer::removeGid(const cocos2d::Vec2 &tileCoordinate){
    int gid = getTileGIDAt(tileCoordinate);
    
    if (gid)
    {
        int z = tileCoordinate.x + tileCoordinate.y * _layerSize.width;
        _tiles[z] = 0;
    }
}

//CCTMXLayer - obtaining positions, offset
Vec2 TMXLayer::calculateLayerOffset(const Vec2& pos)
{
    Vec2 ret;
    switch (_layerOrientation) 
    {
    case TMXOrientationOrtho:
        ret.set( pos.x * _mapTileSize.width, -pos.y *_mapTileSize.height);
        break;
    case TMXOrientationIso:
        ret.set((_mapTileSize.width /2) * (pos.x - pos.y),
                  (_mapTileSize.height /2 ) * (-pos.x - pos.y));
        break;
    case TMXOrientationHex:
        CCASSERT(pos.isZero(), "offset for hexagonal map not implemented yet");
        break;
    case TMXOrientationStaggered:
        {
            float diffX = 0;
            if ((int)std::abs(pos.y) % 2 == 1)
            {
                diffX = _mapTileSize.width/2;
            }
            ret.set(pos.x * _mapTileSize.width + diffX,
                         (-pos.y) * _mapTileSize.height/2);
        }
        break;
    }
    return ret;    
}

Vec2 TMXLayer::getPositionAt(const Vec2& pos)
{
    Vec2 ret;
    switch (_layerOrientation)
    {
    case TMXOrientationOrtho:
        ret = getPositionForOrthoAt(pos);
        break;
    case TMXOrientationIso:
        ret = getPositionForIsoAt(pos);
        break;
    case TMXOrientationHex:
        ret = getPositionForHexAt(pos);
        break;
    case TMXOrientationStaggered:
        ret = getPositionForStaggeredAt(pos);
        break;
    }
    ret = CC_POINT_PIXELS_TO_POINTS( ret );
    return ret;
}

Vec2 TMXLayer::getTilePositionAt(const cocos2d::Vec2 &positon)
{
    Vec2 tilePosition;
    Vec2 gamePosition = CC_POINT_POINTS_TO_PIXELS(positon);
    
    if(gamePosition.x < 0)
        gamePosition.x = 0;
    
    if(gamePosition.y < 0)
        gamePosition.y = 0;
    
    switch(_layerOrientation)
    {
        case TMXOrientationOrtho:
            tilePosition.x = floorf(gamePosition.x / _mapTileSize.width);
            tilePosition.y = floorf(_layerSize.height - gamePosition.y / _mapTileSize.height - 1);
        break;
        case TMXOrientationIso:
            Mat4 mat(_mapTileSize.width / 2, -(_mapTileSize.width / 2), 0, _layerSize.width * _mapTileSize.width / 2, -(_mapTileSize.height / 2), -(_mapTileSize.height / 2), 0, _mapTileSize.height * _layerSize.height, 0, 0, 1, 0, 0, 0, 0, 1);
            
            Mat4 inverseMat = mat.getInversed();
            
            Vec4 postionVec(gamePosition.x, gamePosition.y, 0, 1);
            tilePosition.x =  floorf(inverseMat.m[0] * postionVec.x + inverseMat.m[4] * postionVec.y + inverseMat.m[8] * postionVec.z + inverseMat.m[12] * postionVec.w);
            tilePosition.y =  floorf(inverseMat.m[1] * postionVec.x + inverseMat.m[5] * postionVec.y + inverseMat.m[9] * postionVec.z + inverseMat.m[13] * postionVec.w);
        break;
    }
    
    return tilePosition;
}

Vec2 TMXLayer::getPositionForOrthoAt(const Vec2& pos)
{
    return Vec2(pos.x * _mapTileSize.width,
                            (_layerSize.height - pos.y - 1) * _mapTileSize.height);
}

Vec2 TMXLayer::getPositionForIsoAt(const Vec2& pos)
{
    return Vec2(_mapTileSize.width /2 * (_layerSize.width + pos.x - pos.y - 1),
                             _mapTileSize.height /2 * ((_layerSize.height * 2 - pos.x - pos.y) - 2));
}

Vec2 TMXLayer::getPositionForHexAt(const Vec2& pos)
{
    float diffY = 0;
    if ((int)pos.x % 2 == 1)
    {
        diffY = -_mapTileSize.height/2 ;
    }

    Vec2 xy(
        pos.x * _mapTileSize.width*3/4,
                            (_layerSize.height - pos.y - 1) * _mapTileSize.height + diffY);
    return xy;
}

Vec2 TMXLayer::getPositionForStaggeredAt(const Vec2 &pos)
{
    float diffX = 0;
    if ((int)pos.y % 2 == 1)
    {
        diffX = _mapTileSize.width/2;
    }
    return Vec2(pos.x * _mapTileSize.width + diffX,
                (_layerSize.height - pos.y - 1) * _mapTileSize.height/2);
}

int TMXLayer::getVertexZForPos(const Vec2& pos)
{
    int ret = 0;
    int maxVal = 0;
    if (_useAutomaticVertexZ)
    {
        switch (_layerOrientation) 
        {
        case TMXOrientationIso:
            maxVal = static_cast<int>(_layerSize.width + _layerSize.height);
            ret = static_cast<int>(-(maxVal - (pos.x + pos.y)));
            break;
        case TMXOrientationOrtho:
            ret = static_cast<int>(-(_layerSize.height-pos.y));
            break;
        case TMXOrientationStaggered:
            ret = static_cast<int>(-(_layerSize.height-pos.y));
            break;
        case TMXOrientationHex:
            CCASSERT(0, "TMX Hexa zOrder not supported");
            break;
        default:
            CCASSERT(0, "TMX invalid value");
            break;
        }
    } 
    else
    {
        ret = _vertexZvalue;
    }
    
    return ret;
}

std::string TMXLayer::getDescription() const
{
    return StringUtils::format("<TMXLayer | tag = %d, size = %d,%d>", _tag, (int)_mapTileSize.width, (int)_mapTileSize.height);
}


NS_CC_END
