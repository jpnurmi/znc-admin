/*
 * Copyright (C) 2015 J-P Nurmi
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <znc/Modules.h>
#include <znc/User.h>
#include <znc/IRCNetwork.h>
#include <znc/Chan.h>
#include <znc/znc.h>
#include <functional>

#if (VERSION_MAJOR < 1) || (VERSION_MAJOR == 1 && VERSION_MINOR < 7)
#error The settings module requires ZNC version 1.7.0 or later.
#endif

enum VarType {
	StringType, BoolType, IntType, DoubleType, ListType
};

static const char* VarTypes[] = {
	"String", "Boolean", "Integer", "Double", "List"
};

template <typename T>
struct Variable
{
	CString name;
	VarType type;
	CString description;
	std::function<CString(const T*)> getter;
	std::function<bool(CUser*, T*, const CString&, CString&)> setter;
	std::function<bool(CUser*, T*, CString&)> resetter;
};

class CSettingsMod : public CModule
{
public:
	MODCONSTRUCTOR(CSettingsMod)
	{
		AddHelpCommand();
		AddCommand("Get", nullptr, "<variable>", "Gets the value of a variable.");
		AddCommand("List", nullptr, "[filter]", "Lists available variables filtered by name or type.");
		AddCommand("Set", nullptr, "<variable> <value>", "Sets the value of a variable.");
		AddCommand("Reset", nullptr, "<variable>", "Resets the value(s) of a variable.");
	}

	void OnModCommand(const CString& sLine) override;
	EModRet OnUserRaw(CString& sLine) override;

	CString GetPrefix() const;
	void SetPrefix(const CString& sPrefix);

protected:
	EModRet OnUserCommand(CUser* pUser, const CString& sTgt, const CString& sLine);
	EModRet OnNetworkCommand(CIRCNetwork* pNetwork, const CString& sTgt, const CString& sLine);
	EModRet OnChanCommand(CChan* pChan, const CString& sTgt, const CString& sLine);

private:
	template <typename V>
	void OnHelpCommand(const CString& sTgt, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnListCommand(T* pTarget, const CString& sTgt, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnGetCommand(T* pTarget, const CString& sTgt, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnSetCommand(T* pTarget, const CString& sTgt, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnResetCommand(T* pTarget, const CString& sTgt, const CString& sLine, const std::vector<V>& vVars);

	CTable FilterCmdTable(const CString& sFilter) const;
	template <typename V>
	CTable FilterVarTable(const std::vector<V>& vVars, const CString& sFilter) const;

	void PutError(const CString& sTgt, const CString& sLine);
	void PutLine(const CString& sTgt, const CString& sLine);
	void PutTable(const CString& sTgt, const CTable& Table);
};

// TODO: expose the default constants needed by the reset methods?

static const std::vector<Variable<CZNC>> GlobalVars = {
	{
		"AnonIPLimit", IntType,
		"The limit of anonymous unidentified connections per IP.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetAnonIPLimit());
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetAnonIPLimit(sVal.ToUInt());
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->SetAnonIPLimit(10);
			return true;
		}
	},
	// TODO: BindHost
	{
		"ConnectDelay", IntType,
		"The number of seconds every IRC connection is delayed.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetConnectDelay());
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetConnectDelay(sVal.ToUInt());
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->SetConnectDelay(5);
			return true;
		}
	},
	{
		"HideVersion", BoolType,
		"Whether the version number is hidden from the web interface and CTCP VERSION replies.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetHideVersion());
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetHideVersion(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->SetHideVersion(false);
			return true;
		}
	},
	{
		"MaxBufferSize", IntType,
		"The maximum playback buffer size. Only admin users can exceed the limit.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetMaxBufferSize());
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetMaxBufferSize(sVal.ToUInt());
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->SetMaxBufferSize(500);
			return true;
		}
	},
	{
		"Motd", ListType,
		"The list of 'message of the day' lines that are sent to clients on connect via notice from *status.",
		[](const CZNC* pZNC) {
			const VCString& vsMotd = pZNC->GetMotd();
			return CString("\n").Join(vsMotd.begin(), vsMotd.end());
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->AddMotd(sVal);
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->ClearMotd();
			return true;
		},
	},
	// TODO: PidFile
	{
		"ProtectWebSessions", BoolType,
		"Whether IP changing during each web session is disallowed.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetProtectWebSessions());
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetProtectWebSessions(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->SetProtectWebSessions(true);
			return true;
		}
	},
	{
		"ServerThrottle", IntType,
		"The number of seconds between connect attempts to the same hostname.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetServerThrottle());
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetServerThrottle(sVal.ToUInt());
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->SetServerThrottle(30);
			return true;
		}
	},
	{
		"Skin", StringType,
		"The default web interface skin.",
		[](const CZNC* pZNC) {
			return pZNC->GetSkinName();
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetSkinName(sVal);
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->SetSkinName("");
			return true;
		}
	},
	// TODO: SSLCertFile
	// TODO: SSLCiphers
	// TODO: SSLProtocols
	{
		"StatusPrefix", StringType,
		"The default prefix for status and module queries.",
		[](const CZNC* pZNC) {
			return pZNC->GetSkinName();
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetSkinName(sVal);
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->SetStatusPrefix("");
			return true;
		}
	},
	{
		"TrustedProxy", ListType,
		"The list of trusted proxies.",
		[](const CZNC* pZNC) {
			const VCString& vsProxies = pZNC->GetTrustedProxies();
			return CString("\n").Join(vsProxies.begin(), vsProxies.end());
		},
		[](CUser* pModifier, CZNC* pZNC, const CString& sVal, CString& sError) {
			SCString ssProxies;
			sVal.Split(" ", ssProxies, false);
			for (const CString& sProxy : ssProxies)
				pZNC->AddTrustedProxy(sProxy);
			return true;
		},
		[](CUser* pModifier, CZNC* pZNC, CString& sError) {
			pZNC->ClearTrustedProxies();
			return true;
		}
	},
};

static const std::vector<Variable<CUser>> UserVars = {
	{
		"Admin", BoolType,
		"Whether the user has admin rights.",
		[](const CUser* pTarget) {
			return CString(pTarget->IsAdmin());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetAdmin(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetAdmin(false);
			return true;
		}
	},
	{
		"Allow", ListType,
		"The list of allowed IPs for the user. Wildcards (*) are supported.",
		[](const CUser* pTarget) {
			const SCString& ssHosts = pTarget->GetAllowedHosts();
			return CString("\n").Join(ssHosts.begin(), ssHosts.end());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			SCString ssHosts;
			sVal.Split(" ", ssHosts, false);
			for (const CString& sHost : ssHosts)
				pTarget->AddAllowedHost(sHost);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->ClearAllowedHosts();
			return true;
		}
	},
	{
		"AltNick", StringType,
		"The default alternate nick.",
		[](const CUser* pTarget) {
			return pTarget->GetAltNick();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetAltNick(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetAltNick("");
			return true;
		}
	},
	{
		"AppendTimestamp", BoolType,
		"Whether timestamps are appended to buffer playback messages.",
		[](const CUser* pTarget) {
			return CString(pTarget->GetTimestampAppend());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetTimestampAppend(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetTimestampAppend(false);
			return true;
		}
	},
	{
		"AutoClearChanBuffer", BoolType,
		"Whether channel buffers are automatically cleared after playback.",
		[](const CUser* pTarget) {
			return CString(pTarget->AutoClearChanBuffer());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetAutoClearChanBuffer(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetAutoClearChanBuffer(true);
			return true;
		}
	},
	{
		"AutoClearQueryBuffer", BoolType,
		"Whether query buffers are automatically cleared after playback.",
		[](const CUser* pTarget) {
			return CString(pTarget->AutoClearQueryBuffer());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetAutoClearQueryBuffer(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetAutoClearQueryBuffer(true);
			return true;
		}
	},
	// TODO: BindHost
	{
		"ChanBufferSize", IntType,
		"The maximum amount of lines stored for each channel playback buffer.",
		[](const CUser* pTarget) {
			return CString(pTarget->GetChanBufferSize());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			unsigned int i = sVal.ToUInt();
			if (!pTarget->SetChanBufferSize(i, pTarget->IsAdmin())) {
				sError = "Setting failed, limit is " + CString(CZNC::Get().GetMaxBufferSize());
				return false;
			}
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetChanBufferSize(50);
			return true;
		},
	},
	{
		"ChanModes", StringType,
		"The default modes ZNC sets when joining an empty channel.",
		[](const CUser* pTarget) {
			return pTarget->GetDefaultChanModes();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetDefaultChanModes(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetDefaultChanModes("");
			return true;
		}
	},
#ifdef HAVE_ICU
	{
		"ClientEncoding", StringType,
		"The default client encoding.",
		[](const CUser* pTarget) {
			return pTarget->GetClientEncoding();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetClientEncoding(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetClientEncoding("");
			return true;
		}
	},
#endif
	{
		"CTCPReply", ListType,
		"A list of CTCP request-reply-pairs. Syntax: <request> <reply>.",
		[](const CUser* pTarget) {
			VCString vsReplies;
			for (const auto& it : pTarget->GetCTCPReplies())
				vsReplies.push_back(it.first + " " + it.second);
			return CString("\n").Join(vsReplies.begin(), vsReplies.end());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			CString sRequest = sVal.Token(0);
			CString sReply = sVal.Token(1, true);
			if (sReply.empty()) {
				if (!pTarget->DelCTCPReply(sRequest.AsUpper())) {
					sError = "unable to remove";
					return false;
				}
			} else {
				if (!pTarget->AddCTCPReply(sRequest, sReply)) {
					sError = "unable to add";
					return false;
				}
			}
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			MCString mReplies = pTarget->GetCTCPReplies();
			for (const auto& it : mReplies)
				pTarget->DelCTCPReply(it.first);
			return true;
		}
	},
	// TODO: DCCBindHost
	{
		"DenyLoadMod", BoolType,
		"Whether the user is denied access to load modules.",
		[](const CUser* pTarget) {
			return CString(pTarget->DenyLoadMod());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			if (!pModifier->IsAdmin()) {
				sError = "access denied";
				return false;
			}
			pTarget->SetDenyLoadMod(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			if (!pModifier->IsAdmin()) {
				sError = "access denied";
				return false;
			}
			pTarget->SetDenyLoadMod(false);
			return true;
		}
	},
	// TODO: DenySetBindHost
	{
		"Ident", StringType,
		"The default ident.",
		[](const CUser* pTarget) {
			return pTarget->GetIdent();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetIdent(sVal);
			return true;
		},
		nullptr
	},
	{
		"JoinTries", IntType,
		"The amount of times channels are attempted to join in case of a failure.",
		[](const CUser* pTarget) {
			return CString(pTarget->JoinTries());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetJoinTries(sVal.ToUInt());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetJoinTries(10);
			return true;
		}
	},
	{
		"MaxJoins", IntType,
		"The maximum number of channels ZNC joins at once.",
		[](const CUser* pTarget) {
			return CString(pTarget->MaxJoins());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetMaxJoins(sVal.ToUInt());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetMaxJoins(0);
			return true;
		}
	},
	{
		"MaxNetworks", IntType,
		"The maximum number of networks the user is allowed to have.",
		[](const CUser* pTarget) {
			return CString(pTarget->MaxNetworks());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			if (!pModifier->IsAdmin()) {
				sError = "access denied";
				return false;
			}
			pTarget->SetMaxNetworks(sVal.ToUInt());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			if (!pModifier->IsAdmin()) {
				sError = "access denied";
				return false;
			}
			pTarget->SetMaxNetworks(1);
			return true;
		}
	},
	{
		"MaxQueryBuffers", IntType,
		"The maximum number of query buffers that are stored.",
		[](const CUser* pTarget) {
			return CString(pTarget->MaxQueryBuffers());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetMaxQueryBuffers(sVal.ToUInt());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetMaxQueryBuffers(50);
			return true;
		}
	},
	{
		"MultiClients", BoolType,
		"Whether multiple clients are allowed to connect simultaneously.",
		[](const CUser* pTarget) {
			return CString(pTarget->MultiClients());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetMultiClients(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetMultiClients(true);
			return true;
		}
	},
	{
		"Nick", StringType,
		"The default primary nick.",
		[](const CUser* pTarget) {
			return pTarget->GetNick();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetNick(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetNick("");
			return true;
		}
	},
	{
		"PrependTimestamp", BoolType,
		"Whether timestamps are prepended to buffer playback messages.",
		[](const CUser* pTarget) {
			return CString(pTarget->GetTimestampPrepend());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetTimestampPrepend(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetTimestampPrepend(true);
			return true;
		}
	},
	{
		"Password", StringType,
		"",
		[](const CUser* pTarget) {
			return CString(".", pTarget->GetPass().size());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			const CString sSalt = CUtils::GetSalt();
			const CString sHash = CUser::SaltedHash(sVal, sSalt);
			pTarget->SetPass(sHash, CUser::HASH_DEFAULT, sSalt);
			return true;
		},
		nullptr
	},
	{
		"QueryBufferSize", IntType,
		"The maximum amount of lines stored for each query playback buffer.",
		[](const CUser* pTarget) {
			return CString(pTarget->GetQueryBufferSize());
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			unsigned int i = sVal.ToUInt();
			if (!pTarget->SetQueryBufferSize(i, pTarget->IsAdmin())) {
				sError = "Setting failed, limit is " + CString(CZNC::Get().GetMaxBufferSize());
				return false;
			}
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetQueryBufferSize(50);
			return true;
		}
	},
	{
		"QuitMsg", StringType,
		"The default quit message ZNC uses when disconnecting or shutting down.",
		[](const CUser* pTarget) {
			return pTarget->GetQuitMsg();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetQuitMsg(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetQuitMsg("");
			return true;
		}
	},
	{
		"RealName", StringType,
		"The default real name.",
		[](const CUser* pTarget) {
			return pTarget->GetRealName();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetRealName(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetRealName("");
			return true;
		}
	},
	{
		"Skin", StringType,
		"The web interface skin.",
		[](const CUser* pTarget) {
			return pTarget->GetSkinName();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetSkinName(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetSkinName("");
			return true;
		}
	},
	{
		"SettingsPrefix", StringType,
		"A settings prefix (in addition to the status prefix) for settings queries.",
		[](const CUser* pTarget) {
			CSettingsMod* pMod = dynamic_cast<CSettingsMod*>(pTarget->GetModules().FindModule("settings"));
			return pMod ? pMod->GetPrefix() : "";
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			CSettingsMod* pMod = dynamic_cast<CSettingsMod*>(pTarget->GetModules().FindModule("settings"));
			if (!pMod) {
				sError = "unable to find the module instance";
				return false;
			}
			pMod->SetPrefix(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			CSettingsMod* pMod = dynamic_cast<CSettingsMod*>(pTarget->GetModules().FindModule("settings"));
			if (!pMod) {
				sError = "unable to find the module instance";
				return false;
			}
			pMod->SetPrefix("*");
			return true;
		}
	},
	{
		"StatusPrefix", StringType,
		"The prefix for status and module queries.",
		[](const CUser* pTarget) {
			return pTarget->GetStatusPrefix();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetStatusPrefix(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetStatusPrefix("*");
			return true;
		}
	},
	{
		"TimestampFormat", StringType,
		"The format of the timestamps used in buffer playback messages.",
		[](const CUser* pTarget) {
			return pTarget->GetTimestampFormat();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetTimestampFormat(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetTimestampFormat("[%H:%M:%S]");
			return true;
		}
	},
	{
		"Timezone", StringType,
		"The timezone used for timestamps in buffer playback messages.",
		[](const CUser* pTarget) {
			return pTarget->GetTimezone();
		},
		[](CUser* pModifier, CUser* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetTimezone(sVal);
			return true;
		},
		[](CUser* pModifier, CUser* pTarget, CString& sError) {
			pTarget->SetTimezone("");
			return true;
		}
	},
};

static const std::vector<Variable<CIRCNetwork>> NetworkVars = {
	{
		"AltNick", StringType,
		"An optional network specific alternate nick used if the primary nick is reserved.",
		[](const CIRCNetwork* pTarget) {
			return pTarget->GetAltNick();
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetAltNick(sVal);
			return true;
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, CString& sError) {
			pTarget->SetAltNick("");
			return true;
		}
	},
	// TODO: BindHost
#ifdef HAVE_ICU
	{
		"Encoding", StringType,
		"An optional network specific client encoding.",
		[](const CIRCNetwork* pTarget) {
			return pTarget->GetEncoding();
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetEncoding(sVal);
			return true;
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, CString& sError) {
			pTarget->SetEncoding("");
			return true;
		}
	},
#endif
	{
		"FloodBurst", IntType,
		"The maximum amount of lines ZNC sends at once.",
		[](const CIRCNetwork* pTarget) {
			return CString(pTarget->GetFloodBurst());
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetFloodBurst(sVal.ToUShort());
			return true;
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, CString& sError) {
			pTarget->SetFloodBurst(4);
			return true;
		}
	},
	{
		"FloodRate", DoubleType,
		"The number of lines per second ZNC sends after reaching the FloodBurst limit.",
		[](const CIRCNetwork* pTarget) {
			return CString(pTarget->GetFloodRate());
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetFloodRate(sVal.ToDouble());
			return true;
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, CString& sError) {
			pTarget->SetFloodRate(1);
			return true;
		}
	},
	{
		"Ident", StringType,
		"An optional network specific ident.",
		[](const CIRCNetwork* pTarget) {
			return pTarget->GetIdent();
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetIdent(sVal);
			return true;
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, CString& sError) {
			pTarget->SetIdent("");
			return true;
		}
	},
	// TODO: IRCConnectEnabled?
	{
		"JoinDelay", IntType,
		"The delay in seconds, until channels are joined after getting connected.",
		[](const CIRCNetwork* pTarget) {
			return CString(pTarget->GetJoinDelay());
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetJoinDelay(sVal.ToUShort());
			return true;
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, CString& sError) {
			pTarget->SetJoinDelay(0);
			return true;
		}
	},
	{
		"Nick", StringType,
		"An optional network specific primary nick.",
		[](const CIRCNetwork* pTarget) {
			return pTarget->GetNick();
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetNick(sVal);
			return true;
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, CString& sError) {
			pTarget->SetNick("");
			return true;
		}
	},
	{
		"QuitMsg", StringType,
		"An optional network specific quit message ZNC uses when disconnecting or shutting down.",
		[](const CIRCNetwork* pTarget) {
			return pTarget->GetQuitMsg();
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetQuitMsg(sVal);
			return true;
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, CString& sError) {
			pTarget->SetQuitMsg("");
			return true;
		}
	},
	{
		"RealName", StringType,
		"An optional network specific real name.",
		[](const CIRCNetwork* pTarget) {
			return pTarget->GetRealName();
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetRealName(sVal);
			return true;
		},
		[](CUser* pModifier, CIRCNetwork* pTarget, CString& sError) {
			pTarget->SetRealName("");
			return true;
		}
	},
};

static const std::vector<Variable<CChan>> ChanVars = {
	{
		"AutoClearChanBuffer", BoolType,
		"Whether the channel buffer is automatically cleared after playback.",
		[](const CChan* pTarget) {
			CString sVal(pTarget->AutoClearChanBuffer());
			if (!pTarget->HasAutoClearChanBufferSet())
				sVal += " (default)";
			return sVal;
		},
		[](CUser* pModifier, CChan* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetAutoClearChanBuffer(sVal.ToBool());
			return true;
		},
		[](CUser* pModifier, CChan* pTarget, CString& sError) {
			pTarget->ResetAutoClearChanBuffer();
			return true;
		}
	},
	{
		"Buffer", IntType,
		"The maximum amount of lines stored for the channel specific playback buffer.",
		[](const CChan* pTarget) {
			CString sVal(pTarget->GetBufferCount());
			if (!pTarget->HasBufferCountSet())
				sVal += " (default)";
			return sVal;
		},
		[](CUser* pModifier, CChan* pTarget, const CString& sVal, CString& sError) {
			if (!pTarget->SetBufferCount(sVal.ToUInt(), pTarget->GetNetwork()->GetUser()->IsAdmin()))
				sError = "Setting failed, the limit is " + CString(CZNC::Get().GetMaxBufferSize());
			return sError.empty();
		},
		[](CUser* pModifier, CChan* pTarget, CString& sError) {
			pTarget->ResetBufferCount();
			return true;
		}
	},
	{
		"Detached", BoolType,
		"Whether the channel is detached.",
		[](const CChan* pTarget) {
			return CString(pTarget->IsDetached());
		},
		[](CUser* pModifier, CChan* pTarget, const CString& sVal, CString& sError) {
			bool b = sVal.ToBool();
			if (b != pTarget->IsDetached()) {
				if (b)
					pTarget->DetachUser();
				else
					pTarget->AttachUser();
			}
			return true;
		},
		[](CUser* pModifier, CChan* pTarget, CString& sError) {
			if (pTarget->IsDetached())
				pTarget->AttachUser();
			return true;
		}
	},
	{
		"Disabled", BoolType,
		"Whether the channel is disabled.",
		[](const CChan* pTarget) {
			return CString(pTarget->IsDisabled());
		},
		[](CUser* pModifier, CChan* pTarget, const CString& sVal, CString& sError) {
			bool b = sVal.ToBool();
			if (b != pTarget->IsDisabled()) {
				if (b)
					pTarget->Disable();
				else
					pTarget->Enable();
			}
			return true;
		},
		[](CUser* pModifier, CChan* pTarget, CString& sError) {
			if (pTarget->IsDisabled())
				pTarget->Enable();
			return true;
		}
	},
	{
		"InConfig", BoolType,
		"Whether the channel is stored in the config file.",
		[](const CChan* pTarget) {
			return CString(pTarget->InConfig());
		},
		[](CUser* pModifier, CChan* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetInConfig(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"Key", StringType,
		"An optional channel key.",
		[](const CChan* pTarget) {
			return pTarget->GetKey();
		},
		[](CUser* pModifier, CChan* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetKey(sVal);
			return true;
		},
		[](CUser* pModifier, CChan* pTarget, CString& sError) {
			pTarget->SetKey("");
			return true;
		}
	},
	{
		"Modes", StringType,
		"An optional set of default channel modes ZNC sets when joining an empty channel.",
		[](const CChan* pTarget) {
			return pTarget->GetDefaultModes();
		},
		[](CUser* pModifier, CChan* pTarget, const CString& sVal, CString& sError) {
			pTarget->SetDefaultModes(sVal);
			return true;
		},
		[](CUser* pModifier, CChan* pTarget, CString& sError) {
			pTarget->SetDefaultModes("");
			return true;
		}
	},
};

CString CSettingsMod::GetPrefix() const
{
	CString sPrefix = GetNV("prefix");
	if (sPrefix.empty())
		sPrefix = GetUser()->GetStatusPrefix();
	return sPrefix;
}

void CSettingsMod::SetPrefix(const CString& sPrefix)
{
	SetNV("prefix", sPrefix);
}

void CSettingsMod::OnModCommand(const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (!GetUser()->IsAdmin() && (sCmd.Equals("Set") || sCmd.Equals("Reset"))) {
		PutError(GetModName(), "access denied.");
		return;
	}

	if (sCmd.Equals("Help")) {
		HandleHelpCommand(sLine);

		const CString sPfx = GetUser()->GetStatusPrefix() + GetPrefix();

		if (sLine.Token(1).empty()) {
			PutModule("To access settings of the current user or network, open a query");
			PutModule("with " + sPfx + "user or " + sPfx + "network, respectively.");
			PutModule("-----");
			PutModule("- user settings: /msg " + sPfx + "user help");
			PutModule("- network settings: /msg " + sPfx + "network help");
			PutModule("-----");
			PutModule("To access settings of a different user (admins only) or a specific");
			PutModule("network, open a query with " + sPfx + "target, where target is the name of");
			PutModule("the user or network. The same applies to channel specific settings.");
			PutModule("-----");
			PutModule("- user settings: /msg " + sPfx + "somebody help");
			PutModule("- network settings: /msg " + sPfx + "freenode help");
			PutModule("- channel settings: /msg " + sPfx + "#znc help");
			PutModule("-----");
			PutModule("It is also possible to access the network settings of a different");
			PutModule("user (admins only), or the channel settings of a different network.");
			PutModule("Combine a user, network and channel name separated by a forward");
			PutModule("slash ('/') character.");
			PutModule("-----");
			PutModule("Advanced examples:");
			PutModule("- network settings of another user: /msg " + sPfx + "somebody/freenode help");
			PutModule("- channel settings of another network: /msg " + sPfx + "freenode/#znc help");
			PutModule("- channel settings of another network of another user: /msg " + sPfx + "somebody/freenode/#znc help");
		}
	} else if (sCmd.Equals("List")) {
		OnListCommand(&CZNC::Get(), GetModName(), sLine, GlobalVars);
	} else if (sCmd.Equals("Get")) {
		OnGetCommand(&CZNC::Get(), GetModName(), sLine, GlobalVars);
	} else if (sCmd.Equals("Set")) {
		OnSetCommand(&CZNC::Get(), GetModName(), sLine, GlobalVars);
	} else if (sCmd.Equals("Reset")) {
		OnResetCommand(&CZNC::Get(), GetModName(), sLine, GlobalVars);
	} else {
		PutError(GetModName(), "unknown command");
	}
}

CModule::EModRet CSettingsMod::OnUserRaw(CString& sLine)
{
	CString sCopy = sLine;
	if (sCopy.StartsWith("@"))
		sCopy = sCopy.Token(1, true);
	if (sCopy.StartsWith(":"))
		sCopy = sCopy.Token(1, true);

	const CString sCmd = sCopy.Token(0);

	if (sCmd.Equals("ZNC") || sCmd.Equals("PRIVMSG")) {
		CString sTgt = sCopy.Token(1);
		const CString sRest = sCopy.Token(2, true).TrimPrefix_n(":");
		const CString sPfx = GetPrefix();

		if (sTgt.TrimPrefix(GetUser()->GetStatusPrefix() + sPfx)) {
			// <user>
			if (sTgt.Equals("user"))
				return OnUserCommand(GetUser(), sPfx + sTgt, sRest);
			if (CUser* pUser = CZNC::Get().FindUser(sTgt))
				return OnUserCommand(pUser, sPfx + sTgt, sRest);

			// <network>
			if (sTgt.Equals("network") && GetNetwork())
				return OnNetworkCommand(GetNetwork(), sPfx + sTgt, sRest);
			if (CIRCNetwork* pNetwork = GetUser()->FindNetwork(sTgt))
				return OnNetworkCommand(pNetwork, sPfx + sTgt, sRest);

			// <#chan>
			if (CChan* pChan = GetNetwork() ? GetNetwork()->FindChan(sTgt) : nullptr)
				return OnChanCommand(pChan, sPfx + sTgt, sRest);

			VCString vsParts;
			sTgt.Split("/", vsParts, false);
			if (vsParts.size() == 2) {
				// <user/network>
				if (CUser* pUser = CZNC::Get().FindUser(vsParts[0])) {
					if (CIRCNetwork* pNetwork = pUser->FindNetwork(vsParts[1])) {
						return OnNetworkCommand(pNetwork, sPfx + sTgt, sRest);
					} else {
						// <user/#chan>
						if (pUser == GetUser()) {
							if (CIRCNetwork* pUserNetwork = GetNetwork()) {
								if (CChan* pChan = pUserNetwork->FindChan(vsParts[1]))
									return OnChanCommand(pChan, sPfx + sTgt, sRest);
							}
						}
						if (pUser->GetNetworks().size() == 1) {
							if (CIRCNetwork* pFirstNetwork = pUser->GetNetworks().front()) {
								if (CChan* pChan = pFirstNetwork->FindChan(vsParts[1]))
									return OnChanCommand(pChan, sPfx + sTgt, sRest);
							}
						}
					}
					PutError(sPfx + sTgt, "unknown (or ambiguous) network or channel");
					return HALT;
				}
				// <network/#chan>
				if (CIRCNetwork* pNetwork = GetUser()->FindNetwork(vsParts[0])) {
					if (CChan* pChan = pNetwork->FindChan(vsParts[1])) {
						return OnChanCommand(pChan, sPfx + sTgt, sRest);
					} else {
						PutError(sPfx + sTgt, "unknown channel");
						return HALT;
					}
				}
			} else if (vsParts.size() == 3) {
				// <user/network/#chan>
				if (CUser* pUser = CZNC::Get().FindUser(vsParts[0])) {
					if (CIRCNetwork* pNetwork = pUser->FindNetwork(vsParts[1])) {
						if (CChan* pChan = pNetwork->FindChan(vsParts[2])) {
							return OnChanCommand(pChan, sPfx + sTgt, sRest);
						} else {
							PutError(sPfx + sTgt, "unknown channel");
							return HALT;
						}
					} else {
						PutError(sPfx + sTgt, "unknown network");
						return HALT;
					}
				}
			}
		}
	}
	return CONTINUE;
}

CModule::EModRet CSettingsMod::OnUserCommand(CUser* pUser, const CString& sTgt, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pUser != GetUser() && !GetUser()->IsAdmin()) {
		PutError(sTgt, "access denied");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sTgt, sLine, UserVars);
	else if (sCmd.Equals("List"))
		OnListCommand(pUser, sTgt, sLine, UserVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(pUser, sTgt, sLine, UserVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(pUser, sTgt, sLine, UserVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(pUser, sTgt, sLine, UserVars);
	else
		PutError(sTgt, "unknown command");

	return HALT;
}

CModule::EModRet CSettingsMod::OnNetworkCommand(CIRCNetwork* pNetwork, const CString& sTgt, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pNetwork->GetUser() != GetUser() && !GetUser()->IsAdmin()) {
		PutError(sTgt, "access denied");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sTgt, sLine, NetworkVars);
	else if (sCmd.Equals("List"))
		OnListCommand(pNetwork, sTgt, sLine, NetworkVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(pNetwork, sTgt, sLine, NetworkVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(pNetwork, sTgt, sLine, NetworkVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(pNetwork, sTgt, sLine, NetworkVars);
	else
		PutError(sTgt, "unknown command");

	return HALT;
}

CModule::EModRet CSettingsMod::OnChanCommand(CChan* pChan, const CString& sTgt, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pChan->GetNetwork()->GetUser() != GetUser() && !GetUser()->IsAdmin()) {
		PutError(sTgt, "access denied");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sTgt, sLine, ChanVars);
	else if (sCmd.Equals("List"))
		OnListCommand(pChan, sTgt, sLine, ChanVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(pChan, sTgt, sLine, ChanVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(pChan, sTgt, sLine, ChanVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(pChan, sTgt, sLine, ChanVars);
	else
		PutError(sTgt, "unknown command");

	return HALT;
}

template <typename V>
void CSettingsMod::OnHelpCommand(const CString& sTgt, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sFilter = sLine.Token(1);

	const CTable Table = FilterCmdTable(sFilter);
	if (!Table.empty())
		PutTable(sTgt, Table);
	else
		PutError(sTgt, "unknown command");
}

template <typename T, typename V>
void CSettingsMod::OnListCommand(T* pTarget, const CString& sTgt, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sFilter = sLine.Token(1);

	const CTable Table = FilterVarTable(vVars, sFilter);
	if (!Table.empty())
		PutTable(sTgt, Table);
	else
		PutError(sTgt, "unknown variable");
}

template <typename T, typename V>
void CSettingsMod::OnGetCommand(T* pTarget, const CString& sTgt, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sVar = sLine.Token(1);

	if (sVar.empty()) {
		PutLine(sTgt, "Usage: Get <variable>");
		return;
	}

	bool bFound = false;
	for (const auto& Var : vVars) {
		if (Var.name.WildCmp(sVar, CString::CaseInsensitive)) {
			VCString vsValues;
			Var.getter(pTarget).Split("\n", vsValues, false);
			if (vsValues.empty()) {
				PutLine(sTgt, Var.name + " = ");
			} else {
				for (const CString& s : vsValues)
					PutLine(sTgt, Var.name + " = " + s);
			}
			bFound = true;
		}
	}

	if (!bFound)
		PutError(sTgt, "unknown variable");
}

template <typename T, typename V>
void CSettingsMod::OnSetCommand(T* pTarget, const CString& sTgt, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sVar = sLine.Token(1);
	const CString sVal = sLine.Token(2, true);

	if (sVar.empty() || sVal.empty()) {
		PutLine(sTgt, "Usage: Set <variable> <value>");
		return;
	}

	bool bFound = false;
	for (const auto& Var : vVars) {
		if (Var.name.WildCmp(sVar, CString::CaseInsensitive)) {
			CString sError;
			if (!Var.setter(GetUser(), pTarget, sVal, sError)) {
				PutError(sTgt, sError);
			} else {
				VCString vsValues;
				Var.getter(pTarget).Split("\n", vsValues, false);
				if (vsValues.empty()) {
					PutLine(sTgt, Var.name + " = ");
				} else {
					for (const CString& s : vsValues)
						PutLine(sTgt, Var.name + " = " + s);
				}
			}
			bFound = true;
		}
	}

	if (!bFound)
		PutError(sTgt, "unknown variable");
}

template <typename T, typename V>
void CSettingsMod::OnResetCommand(T* pTarget, const CString& sTgt, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sVar = sLine.Token(1);

	if (sVar.empty()) {
		PutLine(sTgt, "Usage: Reset <variable>");
		return;
	}

	bool bFound = false;
	for (const auto& Var : vVars) {
		if (Var.name.WildCmp(sVar, CString::CaseInsensitive)) {
			CString sError;
			if (!Var.resetter) {
				PutError(sTgt, "reset not supported");
			} else if (!Var.resetter(GetUser(), pTarget, sError)) {
				PutError(sTgt, sError);
			} else {
				VCString vsValues;
				Var.getter(pTarget).Split("\n", vsValues, false);
				if (vsValues.empty()) {
					PutLine(sTgt, Var.name + " = ");
				} else {
					for (const CString& s : vsValues)
						PutLine(sTgt, Var.name + " = " + s);
				}
			}
			bFound = true;
		}
	}

	if (!bFound)
		PutError(sTgt, "unknown variable");
}

CTable CSettingsMod::FilterCmdTable(const CString& sFilter) const
{
	CTable Table;
	Table.AddColumn("Command");
	Table.AddColumn("Description");

	if (sFilter.empty() || sFilter.Equals("Get")) {
		Table.AddRow();
		Table.SetCell("Command", "Get <variable>");
		Table.SetCell("Description", "Gets the value of a variable.");
	}

	if (sFilter.empty() || sFilter.Equals("List")) {
		Table.AddRow();
		Table.SetCell("Command", "List [filter]");
		Table.SetCell("Description", "Lists available variables filtered by name or type.");
	}

	if (sFilter.empty() || sFilter.Equals("Set")) {
		Table.AddRow();
		Table.SetCell("Command", "Set <variable> <value>");
		Table.SetCell("Description", "Sets the value of a variable.");
	}

	if (sFilter.empty() || sFilter.Equals("Reset")) {
		Table.AddRow();
		Table.SetCell("Command", "Reset <variable> <value>");
		Table.SetCell("Description", "Resets the value(s) of a variable.");
	}

	return Table;
}

template <typename V>
CTable CSettingsMod::FilterVarTable(const std::vector<V>& vVars, const CString& sFilter) const
{
	CTable Table;
	Table.AddColumn("Variable");
	Table.AddColumn("Description");

	for (const auto& Var : vVars) {
		CString sType(VarTypes[Var.type]);
		if (sFilter.empty() || sType.Equals(sFilter) || Var.name.StartsWith(sFilter) || Var.name.WildCmp(sFilter, CString::CaseInsensitive)) {
			Table.AddRow();
			Table.SetCell("Variable", Var.name + " (" + sType + ")");
			Table.SetCell("Description", Var.description);
		}
	}

	return Table;
}

void CSettingsMod::PutError(const CString& sTgt, const CString& sError)
{
	PutLine(sTgt, "Error: " + sError);
}

void CSettingsMod::PutLine(const CString& sTgt, const CString& sLine)
{
	if (CClient* pClient = GetClient())
		pClient->PutModule(sTgt, sLine);
	else if (CIRCNetwork* pNetwork = GetNetwork())
		pNetwork->PutModule(sTgt, sLine);
	else if (CUser* pUser = GetUser())
		pUser->PutModule(sTgt, sLine);
}

void CSettingsMod::PutTable(const CString& sTgt, const CTable& Table)
{
	CString sLine;
	unsigned int i = 0;
	while (Table.GetLine(i++, sLine))
		PutLine(sTgt, sLine);
}

template<> void TModInfo<CSettingsMod>(CModInfo& Info) {
}

USERMODULEDEFS(CSettingsMod, "Access ZNC settings conveniently through IRC.")
