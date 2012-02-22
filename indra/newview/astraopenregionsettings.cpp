/* $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Astra Viewer Source Code
 * @file astraopenregionsettings.cpp
 * @brief Handler for OpenRegionInfo event queue message.
 * Copyright (C) 2011, Patrick Sapinski, Matthew Beardmore
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llhttpnode.h"
#include "hippolimits.h"
#include "llfloatertools.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llsurface.h"
#include "llviewerregion.h"
#include "llviewerobject.h"
#include "llfloaterregioninfo.h"
#include "llfloaterworldmap.h"
#include "lldrawpoolterrain.h"
#include "llvlcomposition.h"

//DEBUG includes
//#include "llsdserialize.h" //LLSDNotationStreamer - for dumping LLSD to string

class OpenRegionInfoUpdate : public LLHTTPNode
{
	/*virtual*/ void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		if (!input || !context || !input.isMap() || !input.has("body"))
		{
			llinfos << "malformed OpenRegionInfo update!" << llendl;	 
			return;
		}

		LLSD body = input["body"];
		//llinfos << "data: " << LLSDNotationStreamer(body) << llendl;	 
		//llinfos << "data: " << LLSDXMLStreamer(body) << llendl;	 	 
		
		//set the default limits/settings for this simulator type, as limits from our
		//previous region may not exist in this one
		gHippoLimits->setLimits();

		//limits are also reset in llviewermessage.cpp when we detect a new server
		//version, in case the destination simulator does not support OpenRegionInfo.

		BOOL limitschanged = FALSE;

		if ( body.has("AllowMinimap") )
		{
			gHippoLimits->mAllowMinimap = body["AllowMinimap"].asInteger() == 1;
		}
		/*if ( body.has("AllowPhysicalPrims") )
		{
			gHippoLimits->mAllowPhysicalPrims = body["AllowPhysicalPrims"].asInteger() == 1;
			limitschanged = TRUE;
		}*/
		if ( body.has("DrawDistance") )
		{
			F32 distance = body["DrawDistance"].asReal();
			if (distance > 0)
			{
				gAgentCamera.mDrawDistance = distance;
				//Only if the dd is set
				if ( body.has("ForceDrawDistance") )
				{
					gAgentCamera.mLockedDrawDistance = body["ForceDrawDistance"].asInteger() == 1;
				}
			}
		}
		if ( body.has("LSLFunctions") )
		{
			//IMPLEMENT ME
		}
		/*if ( body.has("TerrainDetailScale") )
		{
			gAgent.getRegion()->getComposition()->setScaleParams(body["TerrainDetailScale"].asReal(), body["TerrainDetailScale"].asReal());

			gHippoLimits->mTerrainScale = body["TerrainDetailScale"].asReal();
			gSavedSettings.setF32("RenderTerrainScale", body["TerrainDetailScale"].asReal());
			LLDrawPoolTerrain::sDetailScale = 1.f/body["TerrainDetailScale"].asReal();
		}*/
		if ( body.has("MinHoleSize") )
		{
			//Note: does NOT update correctly
			gHippoLimits->mMinHoleSize = body["MinHoleSize"].asReal();
			limitschanged = TRUE;
		}
		if ( body.has("MaxHollowSize") )
		{
			//Note: does NOT update correctly
			gHippoLimits->mMaxHollow = body["MaxHollowSize"].asReal();
			limitschanged = TRUE;
		}
		if ( body.has("MaxInventoryItemsTransfer") )
		{
			gHippoLimits->mMaxInventoryItemsTransfer = body["MaxInventoryItemsTransfer"].asReal();
		}
		/*if ( body.has("MaxLinkCount") )
		{
			gHippoLimits->mMaxLinkedPrims = body["MaxLinkCount"].asInteger();
		}
		if ( body.has("MaxLinkCountPhys") )
		{
			gHippoLimits->mMaxPhysLinkedPrims = body["MaxLinkCountPhys"].asInteger();
		}
		if ( body.has("MaxPrimScale") )
		{
			gHippoLimits->mMaxPrimScale = body["MaxPrimScale"].asReal();
			limitschanged = TRUE;
		}
		if ( body.has("MaxPhysPrimScale") )
		{
			//IMPLEMENT ME
		}
		if ( body.has("MinPrimScale") )
		{
			gHippoLimits->mMinPrimScale = body["MinPrimScale"].asReal();
			limitschanged = TRUE;
		}*/
		/*if ( body.has("OffsetOfUTC") )
		{
			gSavedSettings.setS32("TimeOffset", body["OffsetOfUTC"].asInteger());
			gSavedSettings.setBOOL("UseTimeOffset", true);
			ViewerTime::sUseTimeOffset = true;
			ViewerTime::sTimeOffset = gSavedSettings.getS32("TimeOffset");
		}
		if ( body.has("OffsetOfUTCDST") )
		{
			gSavedSettings.setBOOL("TimeOffsetDST", body["OffsetOfUTCDST"].asInteger() == 1 ? TRUE : FALSE);
			ViewerTime::sTimeOffsetDST = gSavedSettings.getBOOL("TimeOffsetDST");
		}*/
		if ( body.has("RenderWater") )
		{
			gHippoLimits->mRenderWater = body["RenderWater"].asInteger() == 1 ? TRUE : FALSE;
			//gAgent.getRegion()->rebuildWater();
		}
		if ( body.has("SayDistance") )
		{
			//gSavedSettings.setU32("ChatDistance", body["SayDistance"].asReal());
		}
		if ( body.has("ShoutDistance") )
		{
			//IMPLEMENT ME
		}
		if ( body.has("WhisperDistance") )
		{
			//IMPLEMENT ME
		}
		if ( body.has("ShowTags") )
		{
			gHippoLimits->mRenderName = body["ShowTags"].asReal();
		}


		//if (limitschanged)
		//	gFloaterTools->updateToolsSizeLimits();
	}
};

LLHTTPRegistration<OpenRegionInfoUpdate>
gHTTPRegistrationOpenRegionInfoUpdate(
	"/message/OpenRegionInfo");