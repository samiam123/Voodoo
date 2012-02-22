// Ported to Imprudence from the Hippo OpenSim Viewer by Jacek Antonelli

#include "llviewerprecompiledheaders.h"

#include "hippogridmanager.h"

#include <cctype>
#include <climits>
#include <cstring>
#include <algorithm>
#include <stdtypes.h>
#include "lldir.h"
#include "lleconomy.h"
#include "llerror.h"
#include "llfile.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llweb.h"

#include "hipporestrequest.h"


// ********************************************************************
// Global Variables

HippoGridManager* gHippoGridManager = 0;

HippoGridInfo HippoGridInfo::FALLBACK_GRIDINFO("");



// ********************************************************************
// ********************************************************************
// HippoGridInfo
// ********************************************************************
// ********************************************************************


// ********************************************************************
// Initialize

HippoGridInfo::HippoGridInfo(const std::string& gridName) :
 //HippoGridInfo::HippoGridInfo(const std::string &gridNick) :
//{
    mPlatform(PLATFORM_OTHER),
	mGridName(gridName),
	mLoginUri(LLStringUtil::null),
	mLoginPage(LLStringUtil::null),
	mHelperUri(LLStringUtil::null),
	mWebSite(LLStringUtil::null),
	mSupportUrl(LLStringUtil::null),
	mRegisterUrl(LLStringUtil::null),
	mPasswordUrl(LLStringUtil::null),
	mSearchUrl(LLStringUtil::null),
	mFirstName(LLStringUtil::null),
	mLastName(LLStringUtil::null),
	mAvatarPassword(LLStringUtil::null),
	mMeshUploadHelper("http://secondlife.com/my/account/mesh.php"),
	mGridMessage(""),
	mXmlState(XML_VOID),
	mVoiceConnector("SLVoice"),
	mRenderCompat(true),
	mInvLinks(false),
	mMaxAgentGroups(-1),
	mCurrencySymbol("OS$"),
	mRealCurrencySymbol("US$"),
	mDirectoryFee(30)
	{
	}
//void HippoGridInfo::setPlatform(Platform platform)
//{
//	mPlatform = platform;
//	if (mPlatform == PLATFORM_SECONDLIFE) mCurrencySymbol = "L$";
//}



// ********************************************************************
// Getters

HippoGridInfo::Platform HippoGridInfo::getPlatform()
{
    return mPlatform;
   //mPlatform = platform;
}

bool HippoGridInfo::isOpenSimulator() const
{
	return (mPlatform == HippoGridInfo::PLATFORM_OPENSIM);
}

bool HippoGridInfo::isSecondLife() const
{
	return (mPlatform == HippoGridInfo::PLATFORM_SECONDLIFE);
}
bool HippoGridInfo::isAurora() const
{
	return (mPlatform == HippoGridInfo::PLATFORM_AURORA);
}
//bool HippoGridInfo::isOther() const
//{
//	return (mPlatform == HippoGridInfo::PLATFORM_OTHER);
//}

const std::string& HippoGridInfo::getGridName() const
{
	return mGridName;
}

const std::string& HippoGridInfo::getGridOwner() const {
	if(isSecondLife()) {
		static const std::string ll = "Linden Lab";
		return ll;
	} else {
		return this->getGridName();
	}	
}

const std::string& HippoGridInfo::getLoginUri() const
{
	return mLoginUri;
}

const std::string& HippoGridInfo::getLoginPage() const
{
	return mLoginPage;
}

const std::string& HippoGridInfo::getHelperUri() const
{
	return mHelperUri;
}

const std::string& HippoGridInfo::getWebSite() const
{
	return mWebSite;
}

const std::string& HippoGridInfo::getSupportUrl() const
{
	return mSupportUrl;
}

const std::string& HippoGridInfo::getRegisterUrl() const
{
	return mRegisterUrl;
}

const std::string& HippoGridInfo::getPasswordUrl() const
{
	return mPasswordUrl;
}

const std::string& HippoGridInfo::getSearchUrl() const
{
	return mSearchUrl;
}

const std::string& HippoGridInfo::getGridMessage() const
{
	return mGridMessage;
}

const std::string& HippoGridInfo::getFirstName() const
{
	return mFirstName;
}

const std::string& HippoGridInfo::getLastName() const
{
	return mLastName;
}

const std::string& HippoGridInfo::getAvatarPassword() const
{
	return mAvatarPassword;
}

bool HippoGridInfo::isRenderCompat() const
{
	return mRenderCompat;
}

const std::string& HippoGridInfo::getCurrencySymbol() const
{
	return mCurrencySymbol;
}

const std::string& HippoGridInfo::getRealCurrencySymbol() const
{
	return mRealCurrencySymbol;
}



// ********************************************************************
// Setters

void HippoGridInfo::setPlatform(Platform platform)
{
	mPlatform = platform;
	if (mPlatform == PLATFORM_SECONDLIFE)
	{
		mCurrencySymbol = "L$";
	}
	else if (mPlatform == PLATFORM_AURORA)
	{
        mCurrencySymbol = "A$";
	}
}


void HippoGridInfo::setPlatform(const std::string& platform)
{
	std::string tmp = platform;
	for (unsigned i=0; i<platform.size(); i++)
		tmp[i] = tolower(tmp[i]);

	if (tmp == "aurora") 
	{
		setPlatform(PLATFORM_AURORA);

	} 
	else if (tmp == "opensim") 
	{
		setPlatform(PLATFORM_OPENSIM);
	} 
	else if (tmp == "secondlife") 
	{
		setPlatform(PLATFORM_SECONDLIFE);
	} 
	else 
    {
		setPlatform(PLATFORM_OTHER);
		llwarns << "Unknown platform '" << platform << "' for " << mGridName << "." << llendl;
	}
}

void HippoGridInfo::setGridName(const std::string& gridName)
{
	mGridName = gridName;
}

void HippoGridInfo::setLoginUri(const std::string& loginUri)
{
	std::string uri = loginUri;
	mLoginUri = sanitizeUri(uri);
}

void HippoGridInfo::setLoginPage(const std::string& loginPage)
{
	mLoginPage = loginPage;
}

void HippoGridInfo::setHelperUri(const std::string& helperUri)
{
	std::string uri = helperUri;
	mHelperUri = sanitizeUri(uri);
}

void HippoGridInfo::setWebSite(const std::string& website)
{
	mWebSite = website;
}

void HippoGridInfo::setSupportUrl(const std::string& url)
{
	mSupportUrl = url;
}

void HippoGridInfo::setRegisterUrl(const std::string& url)
{
	mRegisterUrl = url;
}

void HippoGridInfo::setPasswordUrl(const std::string& url)
{
	mPasswordUrl = url;
}

void HippoGridInfo::setSearchUrl(const std::string& url)
{
	mSearchUrl = url;
}

void HippoGridInfo::setGridMessage(const std::string& message)
{
	mGridMessage = message;
}

void HippoGridInfo::setFirstName(const std::string& firstName)
{
	mFirstName = firstName;
}

void HippoGridInfo::setLastName(const std::string& lastName)
{
	mLastName = lastName;
}

void HippoGridInfo::setAvatarPassword(const std::string& avatarPassword)
{
	mAvatarPassword = avatarPassword;
}

void HippoGridInfo::setRenderCompat(bool compat)
{
	mRenderCompat = compat;
}

void HippoGridInfo::setCurrencySymbol(const std::string& sym)
{
	mCurrencySymbol = sym.substr(0, 3);
}

void HippoGridInfo::setRealCurrencySymbol(const std::string& sym)
{
	mRealCurrencySymbol = sym.substr(0, 3);
}

void HippoGridInfo::setDirectoryFee(int fee)
{
	mDirectoryFee = fee;
}



// ********************************************************************
// Grid Info

std::string HippoGridInfo::getSearchUrl(SearchType ty, bool is_web) const
{
	// Don't worry about whether or not mSearchUrl is empty here anymore -- MC
	if (is_web)
	{
		if (mPlatform == PLATFORM_SECONDLIFE) 
		{
			// Second Life defaults
			if (ty == SEARCH_ALL_EMPTY) 
			{
				return gSavedSettings.getString("SearchURLDefault");
			} 
			else if (ty == SEARCH_ALL_QUERY) 
			{
				return gSavedSettings.getString("SearchURLQuery");
			} 
			else if (ty == SEARCH_ALL_TEMPLATE) 
			{
				return gSavedSettings.getString("SearchURLSuffix2");
			} 
			else 
			{
				llinfos << "Illegal search URL type " << ty << llendl;
				return "";
			}
		}
		else if (!mSearchUrl.empty())
		{
			// Search url sent to us in the login response
			if (ty == SEARCH_ALL_EMPTY) 
			{
				return (mSearchUrl);
			} 
			else if (ty == SEARCH_ALL_QUERY) 
			{
				return (mSearchUrl + "q=[QUERY]&s=[COLLECTION]&");
			} 
			else if (ty == SEARCH_ALL_TEMPLATE) 
			{
				return "lang=[LANG]&mat=[MATURITY]&t=[TEEN]&region=[REGION]&x=[X]&y=[Y]&z=[Z]&session=[SESSION]";
			} 
			else 
			{
				llinfos << "Illegal search URL type " << ty << llendl;
				return "";
			}
		}
		else if (mPlatform == PLATFORM_OPENSIM)
		{
			// OpenSim and other web search defaults
			if (ty == SEARCH_ALL_EMPTY) 
			{
				return gSavedSettings.getString("SearchURLDefaultOpenSim");
			} 
			else if (ty == SEARCH_ALL_QUERY) 
			{
				return gSavedSettings.getString("SearchURLQueryOpenSim");
			} 
			else if (ty == SEARCH_ALL_TEMPLATE) 
			{
				return gSavedSettings.getString("SearchURLSuffixOpenSim");
			} 
			else 
			{
				llinfos << "Illegal search URL type " << ty << llendl;
				return "";
			}
		}
		return "";
	}
	else if (mPlatform == PLATFORM_AURORA)
	{
        // Use the old search all
        if (ty == SEARCH_ALL_EMPTY) 
		{
            return (mSearchUrl + "panel=All&");
        } 
		else if (ty == SEARCH_ALL_QUERY) 
		{
            return (mSearchUrl + "q=[QUERY]&s=[COLLECTION]&");
        } 
		else if (ty == SEARCH_ALL_TEMPLATE) 
		{
            return "lang=[LANG]&m=[MATURITY]&t=[TEEN]&region=[REGION]&x=[X]&y=[Y]&z=[Z]&session=[SESSION]";
        } 
		else 
		{
            llinfos << "Illegal search URL type " << ty << llendl;
            return "";
        }
    }
	return "";
}


//static
void HippoGridInfo::onXmlElementStart(void* userData, const XML_Char* name, const XML_Char** atts)
{
	HippoGridInfo* self = (HippoGridInfo*)userData;
	if (strcasecmp(name, "gridname") == 0)
		self->mXmlState = XML_GRIDNAME;
	else if (strcasecmp(name, "platform") == 0)
		self->mXmlState = XML_PLATFORM;
	else if ((strcasecmp(name, "login") == 0) || (strcasecmp(name, "loginuri") == 0))
		self->mXmlState = XML_LOGINURI;
	else if ((strcasecmp(name, "welcome") == 0) || (strcasecmp(name, "loginpage") == 0))
		self->mXmlState = XML_LOGINPAGE;
	else if ((strcasecmp(name, "economy") == 0) || (strcasecmp(name, "helperuri") == 0))
		self->mXmlState = XML_HELPERURI;
	else if ((strcasecmp(name, "about") == 0) || (strcasecmp(name, "website") == 0))
		self->mXmlState = XML_WEBSITE;
	else if ((strcasecmp(name, "help") == 0) || (strcasecmp(name, "support") == 0))
		self->mXmlState = XML_SUPPORT;
	else if ((strcasecmp(name, "register") == 0) || (strcasecmp(name, "account") == 0))
		self->mXmlState = XML_REGISTER;
	else if (strcasecmp(name, "password") == 0)
		self->mXmlState = XML_PASSWORD;
	else if (strcasecmp(name, "search") == 0)
		self->mXmlState = XML_SEARCH;
	else if (strcasecmp(name, "message") == 0)
		self->mXmlState = XML_MESSAGE;
	else if (strcasecmp(name, "meshurlhelper") == 0)
		self->mXmlState = XML_MESHURLHELPER;
}

//static
void HippoGridInfo::onXmlElementEnd(void* userData, const XML_Char* name)
{
	HippoGridInfo* self = (HippoGridInfo*)userData;
	self->mXmlState = XML_VOID;
}

//static
void HippoGridInfo::onXmlCharacterData(void* userData, const XML_Char* s, int len)
{
	HippoGridInfo* self = (HippoGridInfo*)userData;
	switch (self->mXmlState) 
	{
		case XML_PLATFORM: 
		{
			std::string platform(s, len);
			self->setPlatform(platform); 
			break;
		}	

		case XML_LOGINURI: 
		{
			std::string loginuri(s, len);
			self->mLoginUri = sanitizeUri( loginuri );
			break;
		}

		case XML_HELPERURI: 
		{
			std::string helperuri(s, len);
			self->mHelperUri = sanitizeUri( helperuri );
			break;
		}

		case XML_SEARCH: 
		{
			self->mSearchUrl.assign(s, len);
			//sanitizeQueryUrl(mSearchUrl);
			break;
		}

		case XML_GRIDNAME: self->mGridName.assign(s, len); break;
		case XML_LOGINPAGE: self->mLoginPage.assign(s, len); break;
		case XML_WEBSITE: self->mWebSite.assign(s, len); break;
		case XML_SUPPORT: self->mSupportUrl.assign(s, len); break;
		case XML_REGISTER: self->mRegisterUrl.assign(s, len); break;
		case XML_PASSWORD: self->mPasswordUrl.assign(s, len); break;
		case XML_MESSAGE: self->mGridMessage.assign(s, len); break;
		case XML_MESHURLHELPER: self->mMeshUploadHelper.assign(s, len); break;

		case XML_VOID: break;
	}
}


bool HippoGridInfo::retrieveGridInfo()
{
	if (mLoginUri == "") return false;

	// If last character in uri is not "/"
	std::string uri = mLoginUri;
	if (uri.compare(uri.length()-1, 1, "/") != 0) 
	{
	 	uri += '/';
	}
	std::string reply;
	int result = HippoRestRequest::getBlocking(uri + "get_grid_info", &reply);
	if (result != 200) return false;

	llinfos << "Received: " << reply << llendl;

	bool success = true;
	XML_Parser parser = XML_ParserCreate(0);
	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, onXmlElementStart, onXmlElementEnd);
	XML_SetCharacterDataHandler(parser, onXmlCharacterData);
	mXmlState = XML_VOID;
	if (!XML_Parse(parser, reply.data(), reply.size(), TRUE)) 
	{
		llwarns << "XML Parse Error: " << XML_ErrorString(XML_GetErrorCode(parser)) << llendl;
		success = false;
	}
	XML_ParserFree(parser);

	return success;
}


std::string HippoGridInfo::getUploadFee() const
{
	std::string fee;
	formatFee(fee, LLGlobalEconomy::Singleton::getInstance()->getPriceUpload(), true);
	return fee;
}

std::string HippoGridInfo::getGroupCreationFee() const
{
	std::string fee;
	formatFee(fee, LLGlobalEconomy::Singleton::getInstance()->getPriceGroupCreate(), false);
	return fee;
}

std::string HippoGridInfo::getDirectoryFee() const
{
	std::string fee;
	formatFee(fee, mDirectoryFee, true);
	if (fee != LLTrans::getString("hippo_label_free")) fee += "/" + LLTrans::getString("hippo_label_week");
	return fee;
}

void HippoGridInfo::formatFee(std::string &fee, int cost, bool showFree) const
{
	if (showFree && (cost == 0)) 
	{
		fee = LLTrans::getString("hippo_label_free");
	} 
	else 
	{
		fee = llformat("%s%d", getCurrencySymbol().c_str(), cost);
	}
}


// ********************************************************************
// Static Helpers

// static
const char* HippoGridInfo::getPlatformString(Platform platform)
{
	static const char* platformStrings[PLATFORM_LAST] = 
	{
		"Aurora", "OpenSim", "SecondLife", "Other"
	};
    if ((platform < PLATFORM_AURORA) || (platform >= PLATFORM_LAST)) 
        platform = PLATFORM_AURORA;
	return platformStrings[platform];

	//if ((platform < PLATFORM_OTHER) || (platform >= PLATFORM_LAST))
	//	platform = PLATFORM_OTHER;
      //  return platformStrings[platform];
}

// static
std::string HippoGridInfo::sanitizeUri(std::string &uri)
{
	 if (uri.empty()) {
	 	return "";
	 }

	// // If last character in uri is not "/"
	// // NOTE: This wrongly assumes that all URIs should end with "/"!
	 if (uri.compare(uri.length()-1, 1, "/") != 0) {
	 	return uri + '/';
	 }

	return uri;
}


void HippoGridInfo::initFallback()
{
	FALLBACK_GRIDINFO.setPlatform(PLATFORM_AURORA);
	FALLBACK_GRIDINFO.setGridName("Local Host");
	FALLBACK_GRIDINFO.setLoginUri("http://127.0.0.1:9000/");
	FALLBACK_GRIDINFO.setHelperUri("http://127.0.0.1:9000/");
}

bool HippoGridInfo::supportsInvLinks(){
	if(isSecondLife())
		return true;
	else
		return mInvLinks;
}

void HippoGridInfo::setSupportsInvLinks(bool b) {
	if (b == true && mInvLinks == false)
	{
		llinfos << "Inventory Link support detected" << llendl;
	}
	mInvLinks = b;
}

// ********************************************************************
// ********************************************************************
// HippoGridManager
// ********************************************************************
// ********************************************************************


// ********************************************************************
// Initialize

HippoGridManager::HippoGridManager() :
    mConnectedGrid(0),
    mDefaultGridsVersion(0),
	mCurrentGrid("Local Host"),
	mDefaultGrid("Local Host")
{
}

HippoGridManager::~HippoGridManager()
{
	cleanup();
}


void HippoGridManager::cleanup()
{
	std::map<std::string, HippoGridInfo*>::iterator it, end = mGridInfo.end();
	for (it=mGridInfo.begin(); it != end; ++it) 
	{
		delete it->second;
	}
	mGridInfo.clear();
}


void HippoGridManager::init()
{
	HippoGridInfo::initFallback();
	loadFromFile();

	// !!!### gSavedSettings.getControl("CmdLineLoginURI");
	// !!!### gSavedSettings.getString("CmdLineLoginPage");
	// !!!### gSavedSettings.getString("CmdLineHelperURI");
	// !!!### LLString::compareInsensitive(gGridInfo[grid_index].mLabel, grid_name.c_str()))
}


void HippoGridManager::discardAndReload()
{
	cleanup();
	loadFromFile();
}


// ********************************************************************
// Public Access

HippoGridInfo* HippoGridManager::getGrid(const std::string& grid) const
{
	std::map<std::string, HippoGridInfo*>::const_iterator it;
	it = mGridInfo.find(grid);
	if (it != mGridInfo.end()) 
	{
		return it->second;
	} 
	else 
	{
		return 0;
	}
}


HippoGridInfo* HippoGridManager::getConnectedGrid() const
{
	return (mConnectedGrid)? mConnectedGrid: getCurrentGrid();
}


HippoGridInfo* HippoGridManager::getCurrentGrid() const
{
	HippoGridInfo* grid = getGrid(mCurrentGrid);
	if (grid) 
	{
		return grid;
	} 
	else 
	{
		return &HippoGridInfo::FALLBACK_GRIDINFO;
	}
}

const std::string& HippoGridManager::getDefaultGridNick() const
{
	return mDefaultGrid;
}

const std::string& HippoGridManager::getCurrentGridNick() const
{
	if (mCurrentGrid.empty())
	{
		return mDefaultGrid;
	}
	return mCurrentGrid;
}

void HippoGridManager::setCurrentGridAsConnected()
{
	mConnectedGrid = getCurrentGrid();
}


void HippoGridManager::addGrid(HippoGridInfo* grid)
{
	if (!grid) return;
	const std::string& nick = grid->getGridName();
	if (nick == "") 
	{
		llwarns << "Ignoring to try adding grid with empty nick." << llendl;
		delete grid;
		return;
	}
	if (mGridInfo.find(nick) != mGridInfo.end()) 
	{
		llwarns << "Ignoring to try adding existing grid " << nick << '.' << llendl;
		delete grid;
		return;
	}
	mGridInfo[nick] = grid;
}


void HippoGridManager::deleteGrid(const std::string& grid)
{
	GridIterator it = mGridInfo.find(grid);
	if (it == mGridInfo.end()) {
		llwarns << "Trying to delete non-existing grid " << grid << '.' << llendl;
		return;
	}
	mGridInfo.erase(it);
	llinfos << "Number of grids now: " << mGridInfo.size() << llendl;
	if (mGridInfo.empty()) llinfos << "Grid info map is empty." << llendl;
	if (grid == mDefaultGrid)
		setDefaultGrid(LLStringUtil::null);  // sets first grid, if map not empty
	if (grid == mCurrentGrid)
		mCurrentGrid = mDefaultGrid;
}


void HippoGridManager::setDefaultGrid(const std::string& grid)
{
	GridIterator it = mGridInfo.find(grid);
	if (it != mGridInfo.end()) 
	{
		mDefaultGrid = grid;
	} 
	else if (mGridInfo.find("Talon") != mGridInfo.end()) 
	{
		mDefaultGrid = "Talon";
	} 
	else if (!mGridInfo.empty()) 
	{
        mDefaultGrid = mGridInfo.begin()->first;
	} 
	else 
	{
		mDefaultGrid = "";
	}
}


void HippoGridManager::setCurrentGrid(const std::string& grid)
{
	GridIterator it = mGridInfo.find(grid);
	if (it != mGridInfo.end()) 
	{
		mCurrentGrid = grid;
	} 
	else if (!mGridInfo.empty()) 
	{
		llwarns << "Unknown grid '" << grid << "'. Setting to default grid." << llendl;
        mCurrentGrid = mDefaultGrid;
	}
}


// ********************************************************************
// Persistent Store

void HippoGridManager::loadFromFile()
{
	mDefaultGridsVersion = 0;
	// load user grid info
	parseFile(gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "grids_sg1.xml"), false);
	// merge default grid info, if newer. Force load, if list of grids is empty.
	parseFile(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "default_grids.xml"), !mGridInfo.empty());
	// merge grid info from web site, if newer. Force load, if list of grids is empty.
	if (gSavedSettings.getBOOL("CheckForGridUpdates"))
		parseUrl(gSavedSettings.getString("GridUpdateList"), !mGridInfo.empty());

	std::string last_grid = gSavedSettings.getString("LastSelectedGrid");
	if (last_grid.empty()) last_grid = gSavedSettings.getString("DefaultGrid");
	setDefaultGrid(last_grid);
	setCurrentGrid(last_grid);
}


void HippoGridManager::parseUrl(const std::string url, bool mergeIfNewer)
{
	llinfos << "Loading grid info from '" << url << "'." << llendl;

	// query update server
	std::string escaped_url = LLWeb::escapeURL(url);
	LLSD response = LLHTTPClient::blockingGet(url);

	// check response, return on error
	S32 status = response["status"].asInteger();
	if ((status != 200) || !response["body"].isArray()) 
	{
		llinfos << "GridInfo Update failed (" << status << "): "
			<< (response["body"].isString()? response["body"].asString(): "<unknown error>")
			<< llendl;
		return;
	}

	LLSD gridInfo = response["body"];
	parseData(gridInfo, mergeIfNewer);
}

void HippoGridManager::parseFile(const std::string& fileName, bool mergeIfNewer)
{
	llifstream infile;
	infile.open(fileName.c_str());
	if (!infile.is_open()) 
	{
		llwarns << "Cannot find grid info file " << fileName << " to load." << llendl;
		return;
	}

	LLSD gridInfo;
	if (LLSDSerialize::fromXML(gridInfo, infile) <= 0) 
	{
		llwarns << "Unable to parse grid info file " << fileName << '.' << llendl;		
		return;
	}

	llinfos << "Loading grid info file " << fileName << '.' << llendl;
	parseData(gridInfo, mergeIfNewer);
}


void HippoGridManager::parseData(LLSD &gridInfo, bool mergeIfNewer)
{
	if (mergeIfNewer) 
	{
		LLSD::array_const_iterator it, end = gridInfo.endArray();
		for (it = gridInfo.beginArray(); it != end; ++it) 
		{
			LLSD gridMap = *it;
			if (gridMap.has("default_grids_version")) 
			{
				int version = gridMap["default_grids_version"];
				if (version <= mDefaultGridsVersion) return;
				else break;
			}
		}
		if (it == end) 
		{
			llwarns << "Grid data has no version number." << llendl;
			return;
		}
	}

	llinfos << "Loading grid data." << llendl;

	LLSD::array_const_iterator it, end = gridInfo.endArray();
	for (it = gridInfo.beginArray(); it != end; ++it) 
	{
		LLSD gridMap = *it;
		if (gridMap.has("default_grids_version")) 
		{
			mDefaultGridsVersion = gridMap["default_grids_version"];
		} 
		else if (gridMap.has("gridname") && gridMap.has("loginuri")) 
		{
			std::string gridname = gridMap["gridname"];
			HippoGridInfo* grid;
			GridIterator it = mGridInfo.find(gridname);
			bool newGrid = (it == mGridInfo.end());
			if (newGrid) 
			{
				// create new grid info
				grid = new HippoGridInfo(gridname);
			} 
			else 
			{
				// update existing grid info
				grid = it->second;
			}
			grid->setLoginUri(gridMap["loginuri"]);
			if (gridMap.has("platform")) grid->setPlatform(gridMap["platform"]);
			if (gridMap.has("gridname")) grid->setGridName(gridMap["gridname"]);
			if (gridMap.has("loginpage")) grid->setLoginPage(gridMap["loginpage"]);
			if (gridMap.has("helperuri")) grid->setHelperUri(gridMap["helperuri"]);
			if (gridMap.has("website")) grid->setWebSite(gridMap["website"]);
			if (gridMap.has("support")) grid->setSupportUrl(gridMap["support"]);
			if (gridMap.has("register")) grid->setRegisterUrl(gridMap["register"]);
			if (gridMap.has("password")) grid->setPasswordUrl(gridMap["password"]);
			if (gridMap.has("search")) grid->setSearchUrl(gridMap["search"]);
			if (gridMap.has("render_compat")) grid->setRenderCompat(gridMap["render_compat"]);
			if (gridMap.has("inventory_links")) grid->setSupportsInvLinks(gridMap["inventory_links"]);			
			// if (gridMap.has("firstname")) grid->setFirstName(gridMap["firstname"]);
			// if (gridMap.has("lastname")) grid->setLastName(gridMap["lastname"]);
			// if (gridMap.has("avatarpassword")) grid->setAvatarPassword(gridMap["avatarpassword"]);
			if (newGrid) addGrid(grid);
		}
	}
}


void HippoGridManager::saveFile()
{
	// save default grid to client settings
	gSavedSettings.setString("DefaultGrid", mDefaultGrid);

	// build LLSD
	LLSD gridInfo;
	gridInfo[0]["default_grids_version"] = mDefaultGridsVersion;

	// add grids
	S32 i = 1;
	GridIterator it, end = mGridInfo.end();
	for (it = mGridInfo.begin(); it != end; ++it, i++) 
	{
		HippoGridInfo* grid = it->second;
		//gridInfo[i]["platform"] = HippoGridInfo::getPlatformString(grid->getPlatform());
		gridInfo[i]["platform"] = grid->getPlatform();
		gridInfo[i]["gridname"] = grid->getGridName();
		gridInfo[i]["loginuri"] = grid->getLoginUri();
		gridInfo[i]["loginpage"] = grid->getLoginPage();
		gridInfo[i]["helperuri"] = grid->getHelperUri();
		gridInfo[i]["website"] = grid->getWebSite();
		gridInfo[i]["support"] = grid->getSupportUrl();
		gridInfo[i]["register"] = grid->getRegisterUrl();
		gridInfo[i]["password"] = grid->getPasswordUrl();
		// gridInfo[i]["firstname"] = grid->getFirstName();
		// gridInfo[i]["lastname"] = grid->getLastName();
		// gridInfo[i]["avatarpassword"] = grid->getAvatarPassword();
		
		gridInfo[i]["search"] = grid->getSearchUrl();
		gridInfo[i]["render_compat"] = grid->isRenderCompat();
		gridInfo[i]["inventory_links"] = grid->supportsInvLinks();
	}

	// write client grid info file
	std::string fileName = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "grids_sg1.xml");
	llofstream file;
	file.open(fileName.c_str());
	if (file.is_open()) 
	{
		LLSDSerialize::toPrettyXML(gridInfo, file);
		file.close();
		llinfos << "Saved grids to " << fileName << llendl;
	} 
	else 
	{
		llwarns << "Unable to open grid info file for save: " << fileName << llendl;
	}
}
