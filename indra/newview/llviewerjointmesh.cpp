/** 
 * @file llviewerjointmesh.cpp
 * @brief Implementation of LLViewerJointMesh class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerprecompiledheaders.h"

#include "imageids.h"
#include "llfasttimer.h"
#include "llrender.h"

#include "llapr.h"
#include "llbox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldynamictexture.h"
#include "llface.h"
#include "llgldbg.h"
#include "llglheaders.h"
#include "lltexlayer.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llviewerjointmesh.h"
#include "llvoavatar.h"
#include "llsky.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llmath.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "llmatrix4a.h"

#if !LL_DARWIN && !LL_LINUX && !LL_SOLARIS
extern PFNGLWEIGHTPOINTERARBPROC glWeightPointerARB;
extern PFNGLWEIGHTFVARBPROC glWeightfvARB;
extern PFNGLVERTEXBLENDARBPROC glVertexBlendARB;
#endif

static LLPointer<LLVertexBuffer> sRenderBuffer = NULL;
static const U32 sRenderMask = LLVertexBuffer::MAP_VERTEX |
							   LLVertexBuffer::MAP_NORMAL |
							   LLVertexBuffer::MAP_TEXCOORD0;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLViewerJointMesh::LLSkinJoint
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLSkinJoint
//-----------------------------------------------------------------------------
LLSkinJoint::LLSkinJoint()
{
	mJoint       = NULL;
}

//-----------------------------------------------------------------------------
// ~LLSkinJoint
//-----------------------------------------------------------------------------
LLSkinJoint::~LLSkinJoint()
{
	mJoint = NULL;
}


//-----------------------------------------------------------------------------
// LLSkinJoint::setupSkinJoint()
//-----------------------------------------------------------------------------
BOOL LLSkinJoint::setupSkinJoint( LLViewerJoint *joint)
{
	// find the named joint
	mJoint = joint;
	if ( !mJoint )
	{
		llinfos << "Can't find joint" << llendl;
	}

	// compute the inverse root skin matrix
	mRootToJointSkinOffset.clearVec();

	LLVector3 rootSkinOffset;
	while (joint)
	{
		rootSkinOffset += joint->getSkinOffset();
		joint = (LLViewerJoint*)joint->getParent();
	}

	mRootToJointSkinOffset = -rootSkinOffset;
	mRootToParentJointSkinOffset = mRootToJointSkinOffset;
	mRootToParentJointSkinOffset += mJoint->getSkinOffset();

	return TRUE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLViewerJointMesh
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

BOOL LLViewerJointMesh::sPipelineRender = FALSE;
EAvatarRenderPass LLViewerJointMesh::sRenderPass = AVATAR_RENDER_PASS_SINGLE;
U32 LLViewerJointMesh::sClothingMaskImageName = 0;
LLColor4 LLViewerJointMesh::sClothingInnerColor;

//-----------------------------------------------------------------------------
// LLViewerJointMesh()
//-----------------------------------------------------------------------------
LLViewerJointMesh::LLViewerJointMesh()
	:
	mTexture( NULL ),
	mLayerSet( NULL ),
	mTestImageName( 0 ),
	mFaceIndexCount(0),
	mIsTransparent(FALSE)
{

	mColor[0] = 1.0f;
	mColor[1] = 1.0f;
	mColor[2] = 1.0f;
	mColor[3] = 1.0f;
	mShiny = 0.0f;
	mCullBackFaces = TRUE;

	mMesh = NULL;

	mNumSkinJoints = 0;
	mSkinJoints = NULL;

	mFace = NULL;

	mMeshID = 0;
	mUpdateXform = FALSE;

	mValid = FALSE;
}


//-----------------------------------------------------------------------------
// ~LLViewerJointMesh()
// Class Destructor
//-----------------------------------------------------------------------------
LLViewerJointMesh::~LLViewerJointMesh()
{
	mMesh = NULL;
	mTexture = NULL;
	freeSkinData();
}


//-----------------------------------------------------------------------------
// LLViewerJointMesh::allocateSkinData()
//-----------------------------------------------------------------------------
BOOL LLViewerJointMesh::allocateSkinData( U32 numSkinJoints )
{
	mSkinJoints = new LLSkinJoint[ numSkinJoints ];
	mNumSkinJoints = numSkinJoints;
	return TRUE;
}

//-----------------------------------------------------------------------------
// LLViewerJointMesh::freeSkinData()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::freeSkinData()
{
	mNumSkinJoints = 0;
	delete [] mSkinJoints;
	mSkinJoints = NULL;
}

//--------------------------------------------------------------------
// LLViewerJointMesh::getColor()
//--------------------------------------------------------------------
void LLViewerJointMesh::getColor( F32 *red, F32 *green, F32 *blue, F32 *alpha )
{
	*red   = mColor[0];
	*green = mColor[1];
	*blue  = mColor[2];
	*alpha = mColor[3];
}

//--------------------------------------------------------------------
// LLViewerJointMesh::setColor()
//--------------------------------------------------------------------
void LLViewerJointMesh::setColor( F32 red, F32 green, F32 blue, F32 alpha )
{
	mColor[0] = red;
	mColor[1] = green;
	mColor[2] = blue;
	mColor[3] = alpha;
}


//--------------------------------------------------------------------
// LLViewerJointMesh::getTexture()
//--------------------------------------------------------------------
//LLViewerTexture *LLViewerJointMesh::getTexture()
//{
//	return mTexture;
//}

//--------------------------------------------------------------------
// LLViewerJointMesh::setTexture()
//--------------------------------------------------------------------
void LLViewerJointMesh::setTexture( LLViewerTexture *texture )
{
	mTexture = texture;

	// texture and dynamic_texture are mutually exclusive
	if( texture )
	{
		mLayerSet = NULL;
		//texture->bindTexture(0);
		//texture->setClamp(TRUE, TRUE);
	}
}

//--------------------------------------------------------------------
// LLViewerJointMesh::setLayerSet()
// Sets the shape texture (takes precedence over normal texture)
//--------------------------------------------------------------------
void LLViewerJointMesh::setLayerSet( LLTexLayerSet* layer_set )
{
	mLayerSet = layer_set;
	
	// texture and dynamic_texture are mutually exclusive
	if( layer_set )
	{
		mTexture = NULL;
	}
}



//--------------------------------------------------------------------
// LLViewerJointMesh::getMesh()
//--------------------------------------------------------------------
LLPolyMesh *LLViewerJointMesh::getMesh()
{
	return mMesh;
}

//-----------------------------------------------------------------------------
// LLViewerJointMesh::setMesh()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::setMesh( LLPolyMesh *mesh )
{
	// set the mesh pointer
	mMesh = mesh;

	// release any existing skin joints
	freeSkinData();

	if ( mMesh == NULL )
	{
		return;
	}

	// acquire the transform from the mesh object
	setPosition( mMesh->getPosition() );
	setRotation( mMesh->getRotation() );
	setScale( mMesh->getScale() );

	// create skin joints if necessary
	if ( mMesh->hasWeights() && !mMesh->isLOD())
	{
		U32 numJointNames = mMesh->getNumJointNames();
		
		allocateSkinData( numJointNames );
		std::string *jointNames = mMesh->getJointNames();

		U32 jn;
		for (jn = 0; jn < numJointNames; jn++)
		{
			//llinfos << "Setting up joint " << jointNames[jn] << llendl;
			LLViewerJoint* joint = (LLViewerJoint*)(getRoot()->findJoint(jointNames[jn]) );
			mSkinJoints[jn].setupSkinJoint( joint );
		}
	}

	// setup joint array
	if (!mMesh->isLOD())
	{
		setupJoint((LLViewerJoint*)getRoot());
	}

//	llinfos << "joint render entries: " << mMesh->mJointRenderData.count() << llendl;
}

//-----------------------------------------------------------------------------
// setupJoint()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::setupJoint(LLViewerJoint* current_joint)
{
//	llinfos << "Mesh: " << getName() << llendl;

//	S32 joint_count = 0;
	U32 sj;
	for (sj=0; sj<mNumSkinJoints; sj++)
	{
		LLSkinJoint &js = mSkinJoints[sj];

		if (js.mJoint != current_joint)
		{
			continue;
		}

		// we've found a skinjoint for this joint..

		// is the last joint in the array our parent?
		if(mMesh->mJointRenderData.count() && mMesh->mJointRenderData[mMesh->mJointRenderData.count() - 1]->mWorldMatrix == &current_joint->getParent()->getWorldMatrix())
		{
			// ...then just add ourselves
			LLViewerJoint* jointp = js.mJoint;
			mMesh->mJointRenderData.put(new LLJointRenderData(&jointp->getWorldMatrix(), &js));
//			llinfos << "joint " << joint_count << js.mJoint->getName() << llendl;
//			joint_count++;
		}
		// otherwise add our parent and ourselves
		else
		{
			mMesh->mJointRenderData.put(new LLJointRenderData(&current_joint->getParent()->getWorldMatrix(), NULL));
//			llinfos << "joint " << joint_count << current_joint->getParent()->getName() << llendl;
//			joint_count++;
			mMesh->mJointRenderData.put(new LLJointRenderData(&current_joint->getWorldMatrix(), &js));
//			llinfos << "joint " << joint_count << current_joint->getName() << llendl;
//			joint_count++;
		}
	}

	// depth-first traversal
	for (LLJoint::child_list_t::iterator iter = current_joint->mChildren.begin();
		 iter != current_joint->mChildren.end(); ++iter)
	{
		LLViewerJoint* child_joint = (LLViewerJoint*)(*iter);
		setupJoint(child_joint);
	}
}

const S32 NUM_AXES = 3;

// register layoud
// rotation X 0-n
// rotation Y 0-n
// rotation Z 0-n
// pivot parent 0-n -- child = n+1

static LLMatrix4	gJointMatUnaligned[32];
static LLMatrix4a	gJointMatAligned[32];
static LLMatrix3	gJointRotUnaligned[32];
static LLVector4	gJointPivot[32];

//-----------------------------------------------------------------------------
// uploadJointMatrices()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::uploadJointMatrices()
{
	S32 joint_num;
	LLPolyMesh *reference_mesh = mMesh->getReferenceMesh();
	LLDrawPool *poolp = mFace ? mFace->getPool() : NULL;
	BOOL hardware_skinning = (poolp && poolp->getVertexShaderLevel() > 0) ? TRUE : FALSE;

	//calculate joint matrices
	for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
	{
		LLMatrix4 joint_mat = *reference_mesh->mJointRenderData[joint_num]->mWorldMatrix;

		if (hardware_skinning)
		{
			joint_mat *= LLDrawPoolAvatar::getModelView();
		}
		gJointMatUnaligned[joint_num] = joint_mat;
		gJointRotUnaligned[joint_num] = joint_mat.getMat3();
	}

	BOOL last_pivot_uploaded = FALSE;
	S32 j = 0;

	//upload joint pivots
	for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
	{
		LLSkinJoint *sj = reference_mesh->mJointRenderData[joint_num]->mSkinJoint;
		if (sj)
		{
			if (!last_pivot_uploaded)
			{
				LLVector4 parent_pivot(sj->mRootToParentJointSkinOffset);
				parent_pivot.mV[VW] = 0.f;
				gJointPivot[j++] = parent_pivot;
			}

			LLVector4 child_pivot(sj->mRootToJointSkinOffset);
			child_pivot.mV[VW] = 0.f;

			gJointPivot[j++] = child_pivot;

			last_pivot_uploaded = TRUE;
		}
		else
		{
			last_pivot_uploaded = FALSE;
		}
	}

	//add pivot point into transform
	for (S32 i = 0; i < j; i++)
	{
		LLVector3 pivot;
		pivot = LLVector3(gJointPivot[i]);
		pivot = pivot * gJointRotUnaligned[i];
		gJointMatUnaligned[i].translate(pivot);
	}

	// upload matrices
	if (hardware_skinning)
	{
		GLfloat mat[45*4];
		memset(mat, 0, sizeof(GLfloat)*45*4);

		for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
		{
			gJointMatUnaligned[joint_num].transpose();

			for (S32 axis = 0; axis < NUM_AXES; axis++)
			{
				F32* vector = gJointMatUnaligned[joint_num].mMatrix[axis];
				U32 offset = LL_CHARACTER_MAX_JOINTS_PER_MESH*axis+joint_num;
				memcpy(mat+offset*4, vector, sizeof(GLfloat)*4);
			}
		}
		stop_glerror();
		glUniform4fvARB(gAvatarMatrixParam, 45, mat);
		stop_glerror();
	}
	else
	{
		//load gJointMatUnaligned into gJointMatAligned
		for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); ++joint_num)
		{
			gJointMatAligned[joint_num].loadu(gJointMatUnaligned[joint_num]);
		}
	}
}

//--------------------------------------------------------------------
// LLViewerJointMesh::drawBone()
//--------------------------------------------------------------------
void LLViewerJointMesh::drawBone()
{
}

//--------------------------------------------------------------------
// LLViewerJointMesh::isTransparent()
//--------------------------------------------------------------------
BOOL LLViewerJointMesh::isTransparent()
{
	return mIsTransparent;
}

//--------------------------------------------------------------------
// DrawElementsBLEND and utility code
//--------------------------------------------------------------------

// compate_int is used by the qsort function to sort the index array
int compare_int(const void *a, const void *b)
{
	if (*(U32*)a < *(U32*)b)
	{
		return -1;
	}
	else if (*(U32*)a > *(U32*)b)
	{
		return 1;
	}
	else return 0;
}

//--------------------------------------------------------------------
// LLViewerJointMesh::drawShape()
//--------------------------------------------------------------------
U32 LLViewerJointMesh::drawShape( F32 pixelArea, BOOL first_pass, BOOL is_dummy)
{
	if (!mValid || !mMesh || !mFace || !mVisible || 
		!mFace->getVertexBuffer() ||
		mMesh->getNumFaces() == 0) 
	{
		return 0;
	}

	U32 triangle_count = 0;

	S32 diffuse_channel = LLDrawPoolAvatar::sDiffuseChannel;

	stop_glerror();
	
	//----------------------------------------------------------------
	// setup current color
	//----------------------------------------------------------------
	if (is_dummy)
		glColor4fv(LLVOAvatar::getDummyColor().mV);
	else
		glColor4fv(mColor.mV);

	stop_glerror();
	
	LLGLSSpecular specular(LLColor4(1.f,1.f,1.f,1.f), (mFace->getPool()->getVertexShaderLevel() > 0) ? 0.f : mShiny);

	//----------------------------------------------------------------
	// setup current texture
	//----------------------------------------------------------------
	llassert( !(mTexture.notNull() && mLayerSet) );  // mutually exclusive

	LLTexUnit::eTextureAddressMode old_mode = LLTexUnit::TAM_WRAP;
	if (mTestImageName)
	{
		gGL.getTexUnit(diffuse_channel)->bindManual(LLTexUnit::TT_TEXTURE, mTestImageName);

		if (mIsTransparent)
		{
			glColor4f(1.f, 1.f, 1.f, 1.f);
		}
		else
		{
			glColor4f(0.7f, 0.6f, 0.3f, 1.f);
			gGL.getTexUnit(diffuse_channel)->setTextureColorBlend(LLTexUnit::TBO_LERP_TEX_ALPHA, LLTexUnit::TBS_TEX_COLOR, LLTexUnit::TBS_PREV_COLOR);
		}
	}
	else if( !is_dummy && mLayerSet )
	{
		if(	mLayerSet->hasComposite() )
		{
			gGL.getTexUnit(diffuse_channel)->bind(mLayerSet->getComposite());
		}
		else
		{
			// This warning will always trigger if you've hacked the avatar to show as incomplete.
			// Ignore the warning if that's the case.
			if (!gSavedSettings.getBOOL("RenderUnloadedAvatar"))
			{
				llwarns << "Layerset without composite" << llendl;
			}
			gGL.getTexUnit(diffuse_channel)->bind(LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT));
		}
	}
	else
	if ( !is_dummy && mTexture.notNull() )
	{
		if(mTexture->hasGLTexture())
		{
			old_mode = mTexture->getAddressMode();
		}
		gGL.getTexUnit(diffuse_channel)->bind(mTexture);
		gGL.getTexUnit(diffuse_channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

	}
	else
	{
		gGL.getTexUnit(diffuse_channel)->bind(LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT_AVATAR));
	}
	
	mFace->getVertexBuffer()->setBuffer(sRenderMask);

	U32 start = mMesh->mFaceVertexOffset;
	U32 end = start + mMesh->mFaceVertexCount - 1;
	U32 count = mMesh->mFaceIndexCount;
	U32 offset = mMesh->mFaceIndexOffset;

	if (mMesh->hasWeights())
	{
		if ((mFace->getPool()->getVertexShaderLevel() > 0))
		{
			if (first_pass)
			{
				uploadJointMatrices();
			}
		}
		
		mFace->getVertexBuffer()->drawRange(LLRender::TRIANGLES, start, end, count, offset);
	}
	else
	{
		glPushMatrix();
		LLMatrix4 jointToWorld = getWorldMatrix();
		glMultMatrixf((GLfloat*)jointToWorld.mMatrix);
		mFace->getVertexBuffer()->drawRange(LLRender::TRIANGLES, start, end, count, offset);
		glPopMatrix();
	}
	gPipeline.addTrianglesDrawn(count/3);

	triangle_count += count;
	
	if (mTestImageName)
	{
		gGL.getTexUnit(diffuse_channel)->setTextureBlendType(LLTexUnit::TB_MULT);
	}

	if (mTexture.notNull() && !is_dummy)
	{
		gGL.getTexUnit(diffuse_channel)->bind(mTexture);
		gGL.getTexUnit(diffuse_channel)->setTextureAddressMode(old_mode);
	}

	return triangle_count;
}

//-----------------------------------------------------------------------------
// updateFaceSizes()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::updateFaceSizes(U32 &num_vertices, U32& num_indices, F32 pixel_area)
{
	num_vertices = (num_vertices + 0x3) & ~0x3;
	// Do a pre-alloc pass to determine sizes of data.
	if (mMesh && mValid)
	{
		mMesh->mFaceVertexOffset = num_vertices;
		mMesh->mFaceVertexCount = mMesh->getNumVertices();
		mMesh->mFaceIndexOffset = num_indices;
		mMesh->mFaceIndexCount = mMesh->getSharedData()->mNumTriangleIndices;

		mMesh->getReferenceMesh()->mCurVertexCount = mMesh->mFaceVertexCount;

		num_vertices += mMesh->getNumVertices();
		num_indices += mMesh->mFaceIndexCount;
	}
}

//-----------------------------------------------------------------------------
// updateFaceData()
//-----------------------------------------------------------------------------

void LLViewerJointMesh::updateFaceData(LLFace *face, F32 pixel_area, BOOL damp_wind, bool terse_update)
{
	mFace = face;

	if (!mFace->getVertexBuffer())
	{
		return;
	}

	LLDrawPool *poolp = mFace->getPool();
	BOOL hardware_skinning = (poolp && poolp->getVertexShaderLevel() > 0) ? TRUE : FALSE;

	if (!hardware_skinning && terse_update)
	{ //no need to do terse updates if we're doing software vertex skinning
	 // since mMesh is being copied into mVertexBuffer every frame
		return;
	}


	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> tex_coordsp;
	LLStrider<F32>		 vertex_weightsp;
	LLStrider<LLVector4> clothing_weightsp;
	LLStrider<U16> indicesp;

	// Copy data into the faces from the polymesh data.
	if (mMesh && mValid)
	{
		const U32 num_verts = mMesh->getNumVertices();

		if (num_verts)
		{
			face->getGeometryAvatar(verticesp, normalsp, tex_coordsp, vertex_weightsp, clothing_weightsp);
			face->getVertexBuffer()->getIndexStrider(indicesp);

			verticesp += mMesh->mFaceVertexOffset;
			normalsp += mMesh->mFaceVertexOffset;
			
			//F32* v = (F32*) verticesp.get();
			//F32* n = (F32*) normalsp.get();
			
			//U32 words = num_verts*4;

			//LLVector4a::memcpyNonAliased16(v, (F32*) mMesh->getCoords(), words*sizeof(F32));
			verticesp.assignArray((U8*)mMesh->getCoords(), sizeof(mMesh->getCoords()[0]), num_verts);
			//LLVector4a::memcpyNonAliased16(n, (F32*) mMesh->getNormals(), words*sizeof(F32));
			normalsp.assignArray((U8*)mMesh->getNormals(), sizeof(mMesh->getNormals()[0]), num_verts);

			if (!terse_update)
			{
				vertex_weightsp += mMesh->mFaceVertexOffset;
				clothing_weightsp += mMesh->mFaceVertexOffset;
				tex_coordsp += mMesh->mFaceVertexOffset;
		
				//F32* tc = (F32*) tex_coordsp.get();
				//F32* vw = (F32*) vertex_weightsp.get();
				//F32* cw = (F32*) clothing_weightsp.get();	

				//LLVector4a::memcpyNonAliased16(tc, (F32*) mMesh->getTexCoords(), num_verts*2*sizeof(F32));
				tex_coordsp.assignArray((U8*)mMesh->getTexCoords(), sizeof(mMesh->getTexCoords()[0]), num_verts);
				//LLVector4a::memcpyNonAliased16(vw, (F32*) mMesh->getWeights(), num_verts*sizeof(F32));	
				vertex_weightsp.assignArray((U8*)mMesh->getWeights(), sizeof(mMesh->getWeights()[0]), num_verts);
				//LLVector4a::memcpyNonAliased16(cw, (F32*) mMesh->getClothingWeights(), num_verts*4*sizeof(F32));
				clothing_weightsp.assignArray((U8*)mMesh->getClothingWeights(), sizeof(mMesh->getClothingWeights()[0]), num_verts);
			}

			const U32 idx_count = mMesh->getNumFaces()*3;

				indicesp += mMesh->mFaceIndexOffset;

			U16* __restrict idx = indicesp.get();
			S32* __restrict src_idx = (S32*) mMesh->getFaces();	

			const S32 offset = (S32) mMesh->mFaceVertexOffset;

			for (S32 i = 0; i < (S32)idx_count; ++i)
			{
				*(idx++) = *(src_idx++)+offset;
			}
		}
	}
}



//-----------------------------------------------------------------------------
// updateLOD()
//-----------------------------------------------------------------------------
BOOL LLViewerJointMesh::updateLOD(F32 pixel_area, BOOL activate)
{
	BOOL valid = mValid;
	setValid(activate, TRUE);
	return (valid != activate);
}

// static
void LLViewerJointMesh::updateGeometryOriginal(LLFace *mFace, LLPolyMesh *mMesh)
{
	LLStrider<LLVector3> o_vertices;
	LLStrider<LLVector3> o_normals;

	//get vertex and normal striders
	LLVertexBuffer *buffer = mFace->getVertexBuffer();
	buffer->getVertexStrider(o_vertices,  0);
	buffer->getNormalStrider(o_normals,   0);

	//F32* __restrict vert = o_vertices[0].mV;
	//F32* __restrict norm = o_normals[0].mV;

	const F32* __restrict weights = mMesh->getWeights();
	const LLVector4a* __restrict coords = (LLVector4a*) mMesh->getCoords();
	const LLVector4a* __restrict normals = (LLVector4a*) mMesh->getNormals();

	U32 offset = mMesh->mFaceVertexOffset;
	o_vertices += offset;
	o_normals += offset;

	for (U32 index = 0; index < mMesh->getNumVertices(); index++)
	{
		// equivalent to joint = floorf(weights[index]);
		S32 joint = _mm_cvtt_ss2si(_mm_load_ss(weights+index));
		F32 w = weights[index] - joint;		

		LLMatrix4a gBlendMat;

		if (w != 0.f)
		{
			// blend between matrices and apply
			gBlendMat.setLerp(gJointMatAligned[joint+0],
							  gJointMatAligned[joint+1], w);

			LLVector4a res;
			gBlendMat.affineTransform(coords[index], res);
			(o_vertices++)->set(res.getF32ptr());
			//res.store4a(vert+index*4);
			gBlendMat.rotate(normals[index], res);
			(o_normals++)->set(res.getF32ptr());
			//res.store4a(norm+index*4);
		}
		else
		{  // No lerp required in this case.
			LLVector4a res;
			gJointMatAligned[joint].affineTransform(coords[index], res);
			(o_vertices++)->set(res.getF32ptr());
			//res.store4a(vert+index*4);
			gJointMatAligned[joint].rotate(normals[index], res);
			(o_normals++)->set(res.getF32ptr());
			//res.store4a(norm+index*4);
		}
	}

	buffer->setBuffer(0);
}

const U32 UPDATE_GEOMETRY_CALL_MASK			= 0x1FFF; // 8K samples before overflow
const U32 UPDATE_GEOMETRY_CALL_OVERFLOW		= ~UPDATE_GEOMETRY_CALL_MASK;
static bool sUpdateGeometryCallPointer		= false;
static F64 sUpdateGeometryGlobalTime		= 0.0 ;
static F64 sUpdateGeometryElapsedTime		= 0.0 ;
static F64 sUpdateGeometryElapsedTimeOff	= 0.0 ;
static F64 sUpdateGeometryElapsedTimeOn		= 0.0 ;
static F64 sUpdateGeometryRunAvgOff[10];
static F64 sUpdateGeometryRunAvgOn[10];
static U32 sUpdateGeometryRunCount			= 0 ;
static U32 sUpdateGeometryCalls				= 0 ;
static U32 sUpdateGeometryLastProcessor		= 0 ;
static BOOL sVectorizePerfTest 				= FALSE;
static U32 sVectorizeProcessor 				= 0;

//static
void (*LLViewerJointMesh::sUpdateGeometryFunc)(LLFace* face, LLPolyMesh* mesh);

//static
void LLViewerJointMesh::updateVectorize()
{
	sVectorizePerfTest = gSavedSettings.getBOOL("VectorizePerfTest");
	sVectorizeProcessor = gSavedSettings.getU32("VectorizeProcessor");
	BOOL vectorizeEnable = gSavedSettings.getBOOL("VectorizeEnable");
	BOOL vectorizeSkin = gSavedSettings.getBOOL("VectorizeSkin");

	std::string vp;
	switch(sVectorizeProcessor)
	{
		case 2: vp = "SSE2"; break;					// *TODO: replace the magic #s
		case 1: vp = "SSE"; break;
		default: vp = "COMPILER DEFAULT"; break;
	}
	LL_INFOS("AppInit") << "Vectorization         : " << ( vectorizeEnable ? "ENABLED" : "DISABLED" ) << LL_ENDL ;
	LL_INFOS("AppInit") << "Vector Processor      : " << vp << LL_ENDL ;
	LL_INFOS("AppInit") << "Vectorized Skinning   : " << ( vectorizeSkin ? "ENABLED" : "DISABLED" ) << LL_ENDL ;

	sUpdateGeometryFunc = &updateGeometryOriginal;

	if(vectorizeEnable && vectorizeSkin)
	{
		switch(sVectorizeProcessor)
		{
			case 2:
				if(!supportsSSE2())
					LL_INFOS("AppInit") << "VectorizeProcessor set to unsupported implementation! (SSE2)" << LL_ENDL ;
				else
					sUpdateGeometryFunc = &updateGeometrySSE2;
				break;
			case 1:
				if(!supportsSSE())
					LL_INFOS("AppInit") << "VectorizeProcessor set to unsupported implementation! (SSE)" << LL_ENDL ;
				else
					sUpdateGeometryFunc = &updateGeometrySSE;
				break;
			default:
				if(!gSysCPU.hasAltivec())
					LL_INFOS("AppInit") << "VectorizeProcessor set to unsupported implementation! (Altivec)" << LL_ENDL ;
				else
					sUpdateGeometryFunc = &updateGeometryVectorized;
				break;
		}
	}
}

void LLViewerJointMesh::updateJointGeometry()
{
	if (!(mValid
		  && mMesh
		  && mFace
		  && mMesh->hasWeights()
		  && mFace->getVertexBuffer()
		  && LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) == 0))
	{
		return;
	}

	if (!sVectorizePerfTest)
	{
		// Once we've measured performance, just run the specified
		// code version.
		if(sUpdateGeometryFunc == updateGeometryOriginal)
			uploadJointMatrices();
		sUpdateGeometryFunc(mFace, mMesh);
	}
	else
	{
		// At startup, measure the amount of time in skinning and choose
		// the fastest one.
		LLTimer ug_timer ;
		
		if (sUpdateGeometryCallPointer)
		{
			if(sUpdateGeometryFunc == updateGeometryOriginal)
				uploadJointMatrices();
			// call accelerated version for this processor
			sUpdateGeometryFunc(mFace, mMesh);
		}
		else
		{
			uploadJointMatrices();
			updateGeometryOriginal(mFace, mMesh);
		}
	
		sUpdateGeometryElapsedTime += ug_timer.getElapsedTimeF64();
		++sUpdateGeometryCalls;
		if(0 != (sUpdateGeometryCalls & UPDATE_GEOMETRY_CALL_OVERFLOW))
		{
			F64 time_since_app_start = ug_timer.getElapsedSeconds();
			if(sUpdateGeometryGlobalTime == 0.0 
				|| sUpdateGeometryLastProcessor != sVectorizeProcessor)
			{
				sUpdateGeometryGlobalTime		= time_since_app_start;
				sUpdateGeometryElapsedTime		= 0;
				sUpdateGeometryCalls			= 0;
				sUpdateGeometryRunCount			= 0;
				sUpdateGeometryLastProcessor	= sVectorizeProcessor;
				sUpdateGeometryCallPointer		= false;
				return;
			}
			F64 percent_time_in_function = 
				( sUpdateGeometryElapsedTime * 100.0 ) / ( time_since_app_start - sUpdateGeometryGlobalTime ) ;
			sUpdateGeometryGlobalTime = time_since_app_start;
			if (!sUpdateGeometryCallPointer)
			{
				// First set of run data is with vectorization off.
				sUpdateGeometryCallPointer = true;
				llinfos << "profile (avg of " << sUpdateGeometryCalls << " samples) = "
					<< "vectorize off " << percent_time_in_function
					<< "% of time with "
					<< (sUpdateGeometryElapsedTime / (F64)sUpdateGeometryCalls)
					<< " seconds per call "
					<< llendl;
				sUpdateGeometryRunAvgOff[sUpdateGeometryRunCount] = percent_time_in_function;
				sUpdateGeometryElapsedTimeOff += sUpdateGeometryElapsedTime;
				sUpdateGeometryCalls = 0;
			}
			else
			{
				// Second set of run data is with vectorization on.
				sUpdateGeometryCallPointer = false;
				llinfos << "profile (avg of " << sUpdateGeometryCalls << " samples) = "
					<< "VEC on " << percent_time_in_function
					<< "% of time with "
					<< (sUpdateGeometryElapsedTime / (F64)sUpdateGeometryCalls)
					<< " seconds per call "
					<< llendl;
				sUpdateGeometryRunAvgOn[sUpdateGeometryRunCount] = percent_time_in_function ;
				sUpdateGeometryElapsedTimeOn += sUpdateGeometryElapsedTime;

				sUpdateGeometryCalls = 0;
				sUpdateGeometryRunCount++;
				F64 a = 0.0, b = 0.0;
				for(U32 i = 0; i<sUpdateGeometryRunCount; i++)
				{
					a += sUpdateGeometryRunAvgOff[i];
					b += sUpdateGeometryRunAvgOn[i];
				}
				a /= sUpdateGeometryRunCount;
				b /= sUpdateGeometryRunCount;
				F64 perf_boost = ( sUpdateGeometryElapsedTimeOff - sUpdateGeometryElapsedTimeOn ) / sUpdateGeometryElapsedTimeOn;
				llinfos << "run averages (" << (F64)sUpdateGeometryRunCount
					<< "/10) vectorize off " << a
					<< "% : vectorize type " << sVectorizeProcessor
					<< " " << b
					<< "% : performance boost " 
					<< perf_boost * 100.0
					<< "%"
					<< llendl ;
				if(sUpdateGeometryRunCount == 10)
				{
					// In case user runs test again, force reset of data on
					// next run.
					sUpdateGeometryGlobalTime = 0.0;

					// We have data now on which version is faster.  Switch to that
					// code and save the data for next run.
					gSavedSettings.setBOOL("VectorizePerfTest", FALSE);

					if (perf_boost > 0.0)
					{
						llinfos << "Vectorization improves avatar skinning performance, "
							<< "keeping on for future runs."
							<< llendl;
						gSavedSettings.setBOOL("VectorizeSkin", TRUE);
					}
					else
					{
						// SIMD decreases performance, fall back to original code
						llinfos << "Vectorization decreases avatar skinning performance, "
							<< "switching back to original code."
							<< llendl;

						gSavedSettings.setBOOL("VectorizeSkin", FALSE);
					}
				}
			}
			sUpdateGeometryElapsedTime = 0.0f;
		}
	}
}

void LLViewerJointMesh::dump()
{
	if (mValid)
	{
		llinfos << "Usable LOD " << mName << llendl;
	}
}

// End
