#ifndef __HIPPO_LIMITS_H__
#define __HIPPO_LIMITS_H__


class HippoLimits
{
	LOG_CLASS(HippoLimits);
public:
	HippoLimits();

	int   getMaxAgentGroups() const { return mMaxAgentGroups; }
	float getMaxHeight()      const { return mMaxHeight;      }
	float getMinHoleSize()    const { return mMinHoleSize;    }
	float getMaxHollow()      const { return mMaxHollow;      }
	float getMaxPrimScale()   const { return mMaxPrimScale;   }
    int   getMaxPhysLinkedPrims() const { return mMaxPhysLinkedPrims; }
    float getMaxInventoryItemsTransfer() const { return mMaxInventoryItemsTransfer; }

	void setLimits();

	int   mMaxAgentGroups;
	float mMaxHeight;
	float mMinHoleSize;
	float mMaxHollow;
	float mMaxPrimScale;
 	int   mMaxLinkedPrims;
    int   mMaxPhysLinkedPrims;
	int   mMaxInventoryItemsTransfer;
	int   mRenderName;
	bool  mAllowMinimap;
	bool  mAllowPhysicalPrims;
	bool  mRenderWater;

	void setOpenSimLimits();
	void setSecondLifeLimits();
};


extern HippoLimits *gHippoLimits;


#endif
