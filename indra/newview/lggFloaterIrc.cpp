/* Copyright (c) 2009
 *
 * Greg Hendrickson (LordGregGreg Back) All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "llviewerprecompiledheaders.h"

#include "lggFloaterIrc.h"
#include "lggFloaterIrcEdit.h"
#include "message.h"

#include "llagent.h"
#include "llbutton.h"
#include "llfloaterdirectory.h"
#include "llfocusmgr.h"
#include "llalertdialog.h"
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llimview.h"

// helper functions
void init_irc_list(LLScrollListCtrl* ctrl);

lggPanelIRC::lggPanelIRC() :
	LLPanel()
{
	//gAgent.addListener(this, "new group");
}

lggPanelIRC::~lggPanelIRC()
{
	//gAgent.removeListener(this);
}
bool lggPanelIRC::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	
	return false;
}
// clear the group list, and get a fresh set of info.
void lggPanelIRC::newList()
{
// 	LLCtrlListInterface *group_list = childGetListInterface("IRC_list");
// 	if (group_list)
// 	{
// 		group_list->operateOnAll(LLCtrlListInterface::OP_DELETE);
// 	}
	llinfos << "refreshing..." << llendl;

	init_irc_list(getChild<LLScrollListCtrl>("IRC_list"));
	enableButtons();
}
void lggPanelIRC::initHelpBtn(const std::string& name, const std::string& xml_alert)
{
	childSetAction(name, onClickHelp, new std::string(xml_alert));
}
void lggPanelIRC::onClickHelp(void* data)
{
	std::string* xml_alert = (std::string*)data;
	LLNotifications::instance().add(*xml_alert);
}
BOOL lggPanelIRC::postBuild()
{
	childSetCommitCallback("IRC_list", onIrcList, this);

	//init_irc_list(getChild<LLScrollListCtrl>("IRC_list"));

	childSetAction("IRC_IM", onBtnIM, this);

	childSetAction("IRC_new", onBtnNewIrc, this);

	childSetAction("IRC_edit", onBtnEdit, this);

	childSetAction("IRC_remove", onBtnRemove, this);
	childSetAction("IRC_refresh", onBtnRefresh, this);

	setDefaultBtn("IRC_IM");

	childSetDoubleClickCallback("IRC_list", onBtnIM);
	childSetUserData("IRC_list", this);

	initHelpBtn("IRC_Help",	"Help_IRCSettings");

	glggIrcGroupHandler.setListPanel(this);
	//newList();

	return TRUE;
}

void lggPanelIRC::enableButtons()
{
	LLCtrlListInterface *irc_list = childGetListInterface("IRC_list");
	LLUUID irc_id;
	if (irc_list)
	{
		irc_id = irc_list->getCurrentID();
	}
	if (irc_id.notNull())
	{
		childEnable("IRC_remove");
		childEnable("IRC_IM");
		childEnable("IRC_edit");
	}
	else
	{
		childDisable("IRC_IM");
		childDisable("IRC_edit");
		childDisable("IRC_remove");
	}
	childEnable("IRC_new");
	refresh();
}


void lggPanelIRC::onBtnNewIrc(void* userdata)
{
	llinfos << "lggPanelIRC::onbuttonnewirc" << llendl;
	
	lggPanelIRC* self = (lggPanelIRC*)userdata;
	if(self) self->newirc();
}


void lggPanelIRC::onBtnEdit(void* userdata)
{
	lggPanelIRC* self = (lggPanelIRC*)userdata;
	if(self) self->editirc();
}

void lggPanelIRC::onBtnIM(void* userdata)
{
	lggPanelIRC* self = (lggPanelIRC*)userdata;
	if(self) self->startirc();
}
void lggPanelIRC::onBtnRefresh(void* userdata)
{
	lggPanelIRC* self = (lggPanelIRC*)userdata;
	if(self) self->newList();
}
void lggPanelIRC::onBtnRemove(void* userdata)
{
	lggPanelIRC* self = (lggPanelIRC*)userdata;
	if(self) self->removeirc();
}
////////////////////////////////////
void lggPanelIRC::newirc()
{
	llinfos << "lggPanelIRC::newirc" << llendl;
	//lggPanelIRC::showCreateGroup(NULL);
	LggIrcFloaterStarter::show(lggIrcData(),this);
}
void lggPanelIRC::editirc()
{
	llinfos << "lggPanelIRC::editirc" << llendl;
	LLCtrlListInterface *irc_list = childGetListInterface("IRC_list");
	LLUUID irc_id;

	if (irc_list && (irc_id = irc_list->getCurrentID()).notNull())
	{
		
		LggIrcFloaterStarter::show(glggIrcGroupHandler.getIrcGroupInfoByID(irc_list->getCurrentID()),this);
		//TODO CLeanup shit
		
	}
	
}
void lggPanelIRC::startirc()
{
	LLCtrlListInterface *irc_list = childGetListInterface("IRC_list");
	LLUUID irc_id;

	if (irc_list && (irc_id = irc_list->getCurrentID()).notNull())
	{
		llinfos << " starting irc buttons.." << llendl;
		lggIrcData idat = glggIrcGroupHandler.getIrcGroupInfoByID(irc_list->getCurrentID());
		//delete indat; TODO CLEANUP 
		llinfos << " got idat... it is.." << llendl;
		llinfos << idat.toString() << llendl;//FFFFFFFFFFFFFFFFFFFFFFFFFFFffff
		glggIrcGroupHandler.startUpIRCListener(idat);
		
		
	}
}

void lggPanelIRC::removeirc()
{
	llinfos << "lggPanelIRC::removeirc" << llendl;
	LLCtrlListInterface *irc_list = childGetListInterface("IRC_list");
	LLUUID irc_id;

	if (irc_list && (irc_id = irc_list->getCurrentID()).notNull())
	{
		llinfos << "lggPanelIRC::removeirc" << irc_list->getSelectedValue() << " and " << irc_list->getCurrentID() << llendl;
	
		glggIrcGroupHandler.deleteIrcGroupByID(irc_list->getCurrentID());
		
	}
	newList();
}



void lggPanelIRC::onIrcList(LLUICtrl* ctrl, void* userdata)
{
	lggPanelIRC* self = (lggPanelIRC*)userdata;
	if(self) self->enableButtons();
}

void init_irc_list(LLScrollListCtrl* ctrl)
{
	if(!ctrl)return;
	llinfos << "more refreshing.." << llendl;

	std::vector<lggIrcData> allGroups = glggIrcGroupHandler.getFileNames();
	S32 count = allGroups.size();
	LLUUID id;
	LLCtrlListInterface *irc_list = ctrl->getListInterface();
	if (!irc_list) return;

	irc_list->operateOnAll(LLCtrlListInterface::OP_DELETE);

	for(S32 i = 0; i < count; ++i)
	{
		llinfos << "init_irc_list and " << allGroups[i].name << llendl;
	
		id = allGroups[i].id;//gAgent.mGroups.get(i).mID;
		
		std::string style = "NORMAL";
		LLSD element;
		// comment out one line for now gives an error sams voodoo
		//element["columns"][0]["Color"] = gColors.getColor("DefaultListText").getValue();
		
		element["id"] = id;
		element["columns"][0]["column"] = "IRC_name";
		element["columns"][0]["value"] = allGroups[i].name;
		element["columns"][0]["font"] = "SANSSERIF";
		element["columns"][0]["font-style"] = style;

		irc_list->addElement(element, ADD_SORTED);
		
	}
	//irc_list->selectFirstItem();
}
