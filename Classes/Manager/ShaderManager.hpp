//
//  ShaderManager.hpp
//  ProjectSLG
//
//  Created by shexinxing on 6/12/16.
//
//

#ifndef ShaderManager_hpp
#define ShaderManager_hpp

#include <stdio.h>
#include "Singleton.h"
class ShaderManager : public Singleton<ShaderManager>
{
public:
    enum ShaderType
    {
        kShaderType_PositionTextureColorETC1,
        kShaderType_PositionTextureColorETC1_GrayScale,
        kShaderType_PositionTextureColorETC1_NO_MVP,
        kShaderType_Scrub,
        kShaderType_Scrub_ETC1
    };
    
    ShaderManager();
    virtual ~ShaderManager();
    
    void loadGLPrograms();
    cocos2d::GLProgramState* createProgramState(ShaderType type, cocos2d::Texture2D* texture = nullptr);
    
private:
    void loadDefaultGLProgram(cocos2d::GLProgram *program, int type);
    void genTexCoordOffsets(GLuint width, GLuint height, GLfloat* pOutTexCoordOff, GLfloat step );
};

extern const GLchar * positionTextureColorETC1_frag;
extern const GLchar* positionTextureColorETC1_GrayScale_frag;
extern const GLchar* scrub_frag;
extern const GLchar* scrub_frag_etc1;
#endif /* ShaderManager_hpp */
