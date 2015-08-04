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
#error The settings module requires the latest development version of ZNC.
#endif

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
	EModRet OnGlobalCommand(const CString& sTgt, const CString& sLine);
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

	void PutLine(const CString& sTgt, const CString& sLine);
	void PutTable(const CString& sTgt, const CTable& Table);
};

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
	std::function<bool(T*, const CString&, CString&)> setter;
	std::function<bool(T*, CString&)> resetter;
};

static const std::vector<Variable<CZNC>> GlobalVars = {
	{
		"AnonIPLimit", IntType,
		"The limit of anonymous unidentified connections per IP.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetAnonIPLimit());
		},
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetAnonIPLimit(sVal.ToUInt());
			return true;
		},
		nullptr
	},
	{
		"BindHost", ListType,
		"The list of allowed bindhosts.",
		[](const CZNC* pZNC) {
			const VCString& vsHosts = pZNC->GetBindHosts();
			return CString("\n").Join(vsHosts.begin(), vsHosts.end());
		},
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			VCString ssHosts;
			sVal.Split(" ", ssHosts, false);
			for (const CString& sHost : ssHosts)
				pZNC->AddBindHost(sHost);
			return true;
		},
		[](CZNC* pZNC, CString& sError) {
			pZNC->ClearBindHosts();
			return true;
		},
	},
	{
		"ConnectDelay", IntType,
		"The number of seconds every IRC connection is delayed.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetConnectDelay());
		},
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetConnectDelay(sVal.ToUInt());
			return true;
		},
		nullptr
	},
	{
		"HideVersion", BoolType,
		"Whether the version number is hidden from the web interface and CTCP VERSION replies.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetHideVersion());
		},
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetHideVersion(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"MaxBufferSize", IntType,
		"The maximum playback buffer size. Only admin users can exceed the limit.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetMaxBufferSize());
		},
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetMaxBufferSize(sVal.ToUInt());
			return true;
		},
		nullptr
	},
	{
		"Motd", ListType,
		"The list of 'message of the day' lines that are sent to clients on connect via notice from *status.",
		[](const CZNC* pZNC) {
			const VCString& vsMotd = pZNC->GetMotd();
			return CString("\n").Join(vsMotd.begin(), vsMotd.end());
		},
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->AddMotd(sVal);
			return true;
		},
		[](CZNC* pZNC, CString& sError) {
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
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetProtectWebSessions(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"ServerThrottle", IntType,
		"The number of seconds between connect attempts to the same hostname.",
		[](const CZNC* pZNC) {
			return CString(pZNC->GetServerThrottle());
		},
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetServerThrottle(sVal.ToUInt());
			return true;
		},
		nullptr
	},
	{
		"Skin", StringType,
		"The default web interface skin.",
		[](const CZNC* pZNC) {
			return pZNC->GetSkinName();
		},
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetSkinName(sVal);
			return true;
		},
		nullptr
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
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			pZNC->SetSkinName(sVal);
			return true;
		},
		nullptr
	},
	{
		"TrustedProxy", ListType,
		"The list of trusted proxies.",
		[](const CZNC* pZNC) {
			const VCString& vsProxies = pZNC->GetTrustedProxies();
			return CString("\n").Join(vsProxies.begin(), vsProxies.end());
		},
		[](CZNC* pZNC, const CString& sVal, CString& sError) {
			SCString ssProxies;
			sVal.Split(" ", ssProxies, false);
			for (const CString& sProxy : ssProxies)
				pZNC->AddTrustedProxy(sProxy);
			return true;
		},
		[](CZNC* pZNC, CString& sError) {
			pZNC->ClearTrustedProxies();
			return true;
		}
	},
};

static const std::vector<Variable<CUser>> UserVars = {
	{
		"Admin", BoolType,
		"Whether the user has admin rights.",
		[](const CUser* pUser) {
			return CString(pUser->IsAdmin());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			if (!pUser->IsAdmin()) {
				sError = "Error: access denied";
				return false;
			}

			pUser->SetAdmin(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"Allow", ListType,
		"The list of allowed IPs for the user. Wildcards (*) are supported.",
		[](const CUser* pUser) {
			const SCString& ssHosts = pUser->GetAllowedHosts();
			return CString("\n").Join(ssHosts.begin(), ssHosts.end());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			// TODO: CUser::DelAllowedHost() and/or CUser::SetAllowedHosts()
			SCString ssHosts;
			sVal.Split(" ", ssHosts, false);
			for (const CString& sHost : ssHosts)
				pUser->AddAllowedHost(sHost);
			return true;
		},
		nullptr
	},
	{
		"AltNick", StringType,
		"The default alternate nick.",
		[](const CUser* pUser) {
			return pUser->GetAltNick();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetAltNick(sVal);
			return true;
		},
		nullptr
	},
	{
		"AppendTimestamp", BoolType,
		"Whether timestamps are appended to buffer playback messages.",
		[](const CUser* pUser) {
			return CString(pUser->GetTimestampAppend());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetTimestampAppend(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"AutoClearChanBuffer", BoolType,
		"Whether channel buffers are automatically cleared after playback.",
		[](const CUser* pUser) {
			return CString(pUser->AutoClearChanBuffer());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetAutoClearChanBuffer(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"AutoClearQueryBuffer", BoolType,
		"Whether query buffers are automatically cleared after playback.",
		[](const CUser* pUser) {
			return CString(pUser->AutoClearQueryBuffer());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetAutoClearQueryBuffer(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"BindHost", StringType,
		"The default bind host.",
		[](const CUser* pUser) {
			return pUser->GetBindHost();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			// TODO: move sanity checking to CUser::SetBindHost()
			if (!pUser->DenySetBindHost() && !pUser->IsAdmin()) {
				sError = "Error: access denied";
				return false;
			}

			if (sVal.Equals(pUser->GetBindHost())) {
				sError = "This bind host is already set!";
				return false;
			}

			const VCString& vsHosts = CZNC::Get().GetBindHosts();
			if (!pUser->IsAdmin() && !vsHosts.empty()) {
				bool bFound = false;

				for (const CString& sHost : vsHosts) {
					if (sVal.Equals(sHost)) {
						bFound = true;
						break;
					}
				}

				if (!bFound) {
					sError = "The bind host is not available. See /msg " + pUser->GetStatusPrefix() + "status ListBindHosts for the list of available bind hosts.";
					return false;
				}
			}

			pUser->SetBindHost(sVal);
			return true;
		},
		nullptr
	},
	{
		"ChanBufferSize", IntType,
		"The maximum amount of lines stored for each channel playback buffer.",
		[](const CUser* pUser) {
			return CString(pUser->GetChanBufferSize());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			unsigned int i = sVal.ToUInt();
			if (!pUser->SetChanBufferSize(i, pUser->IsAdmin())) {
				sError = "Setting failed, limit is " + CString(CZNC::Get().GetMaxBufferSize());
				return false;
			}
			return true;
		},
		nullptr
	},
	{
		"ChanModes", StringType,
		"The default modes ZNC sets when joining an empty channel.",
		[](const CUser* pUser) {
			return pUser->GetDefaultChanModes();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetDefaultChanModes(sVal);
			return true;
		},
		nullptr
	},
#ifdef HAVE_ICU
	{
		"ClientEncoding", StringType,
		"The default client encoding.",
		[](const CUser* pUser) {
			return pUser->GetClientEncoding();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetClientEncoding(sVal);
			return true;
		},
		nullptr
	},
#endif
	{
		"CTCPReply", ListType,
		"A list of CTCP request-reply-pairs. Syntax: <request> <reply>.",
		[](const CUser* pUser) {
			VCString vsReplies;
			for (const auto& it : pUser->GetCTCPReplies())
				vsReplies.push_back(it.first + " " + it.second);
			return CString("\n").Join(vsReplies.begin(), vsReplies.end());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			CString sRequest = sVal.Token(0);
			CString sReply = sVal.Token(1, true);
			if (sReply.empty()) {
				if (!pUser->DelCTCPReply(sRequest.AsUpper())) {
					sError = "Error: unable to remove!";
					return false;
				}
			} else {
				if (!pUser->AddCTCPReply(sRequest, sReply)) {
					sError = "Error: unable to add!";
					return false;
				}
			}
			return true;
		},
		[](CUser* pUser, CString& sError) {
			MCString mReplies = pUser->GetCTCPReplies();
			for (const auto& it : mReplies)
				pUser->DelCTCPReply(it.first);
			return true;
		}
	},
	{
		"DCCBindHost", StringType,
		"An optional bindhost for DCC connections.",
		[](const CUser* pUser) {
			return pUser->GetDCCBindHost();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			if (!pUser->IsAdmin()) {
				sError = "Error: access denied";
				return false;
			}
			pUser->SetDCCBindHost(sVal);
			return true;
		},
		nullptr
	},
	{
		"DenyLoadMod", BoolType,
		"Whether the user is denied access to load modules.",
		[](const CUser* pUser) {
			return CString(pUser->DenyLoadMod());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			if (!pUser->IsAdmin()) {
				sError = "Error: access denied";
				return false;
			}
			pUser->SetDenyLoadMod(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"DenySetBindHost", BoolType,
		"Whether the user is denied access to set a bind host.",
		[](const CUser* pUser) {
			return CString(pUser->DenySetBindHost());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			if (!pUser->IsAdmin()) {
				sError = "Error: access denied";
				return false;
			}
			pUser->SetDenySetBindHost(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"Ident", StringType,
		"The default ident.",
		[](const CUser* pUser) {
			return pUser->GetIdent();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetIdent(sVal);
			return true;
		},
		nullptr
	},
	{
		"JoinTries", IntType,
		"The amount of times channels are attempted to join in case of a failure.",
		[](const CUser* pUser) {
			return CString(pUser->JoinTries());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetJoinTries(sVal.ToUInt());
			return true;
		},
		nullptr
	},
	{
		"MaxJoins", IntType,
		"The maximum number of channels ZNC joins at once.",
		[](const CUser* pUser) {
			return CString(pUser->MaxJoins());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetMaxJoins(sVal.ToUInt());
			return true;
		},
		nullptr
	},
	{
		"MaxNetworks", IntType,
		"The maximum number of networks the user is allowed to have.",
		[](const CUser* pUser) {
			return CString(pUser->MaxNetworks());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			if (!pUser->IsAdmin()) {
				sError = "Error: access denied";
				return false;
			}
			pUser->SetMaxNetworks(sVal.ToUInt());
			return true;
		},
		nullptr
	},
	{
		"MaxQueryBuffers", IntType,
		"The maximum number of query buffers that are stored.",
		[](const CUser* pUser) {
			return CString(pUser->MaxQueryBuffers());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetMaxQueryBuffers(sVal.ToUInt());
			return true;
		},
		nullptr
	},
	{
		"MultiClients", BoolType,
		"Whether multiple clients are allowed to connect simultaneously.",
		[](const CUser* pUser) {
			return CString(pUser->MultiClients());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetMultiClients(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"Nick", StringType,
		"The default primary nick.",
		[](const CUser* pUser) {
			return pUser->GetNick();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetNick(sVal);
			return true;
		},
		nullptr
	},
	{
		"PrependTimestamp", BoolType,
		"Whether timestamps are prepended to buffer playback messages.",
		[](const CUser* pUser) {
			return CString(pUser->GetTimestampPrepend());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetTimestampPrepend(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"Password", StringType,
		"",
		[](const CUser* pUser) {
			return CString(".", pUser->GetPass().size());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			const CString sSalt = CUtils::GetSalt();
			const CString sHash = CUser::SaltedHash(sVal, sSalt);
			pUser->SetPass(sHash, CUser::HASH_DEFAULT, sSalt);
			return true;
		},
		nullptr
	},
	{
		"QueryBufferSize", IntType,
		"The maximum amount of lines stored for each query playback buffer.",
		[](const CUser* pUser) {
			return CString(pUser->GetQueryBufferSize());
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			unsigned int i = sVal.ToUInt();
			if (!pUser->SetQueryBufferSize(i, pUser->IsAdmin())) {
				sError = "Setting failed, limit is " + CString(CZNC::Get().GetMaxBufferSize());
				return false;
			}
			return true;
		},
		nullptr
	},
	{
		"QuitMsg", StringType,
		"The default quit message ZNC uses when disconnecting or shutting down.",
		[](const CUser* pUser) {
			return pUser->GetQuitMsg();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetQuitMsg(sVal);
			return true;
		},
		nullptr
	},
	{
		"RealName", StringType,
		"The default real name.",
		[](const CUser* pUser) {
			return pUser->GetRealName();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetRealName(sVal);
			return true;
		},
		nullptr
	},
	{
		"Skin", StringType,
		"The web interface skin.",
		[](const CUser* pUser) {
			return pUser->GetSkinName();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetSkinName(sVal);
			return true;
		},
		nullptr
	},
	{
		"StatusPrefix", StringType,
		"The prefix for status and module queries.",
		[](const CUser* pUser) {
			return pUser->GetStatusPrefix();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetStatusPrefix(sVal);
			return true;
		},
		nullptr
	},
	{
		"TimestampFormat", StringType,
		"The format of the timestamps used in buffer playback messages.",
		[](const CUser* pUser) {
			return pUser->GetTimestampFormat();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetTimestampFormat(sVal);
			return true;
		},
		nullptr
	},
	{
		"Timezone", StringType,
		"The timezone used for timestamps in buffer playback messages.",
		[](const CUser* pUser) {
			return pUser->GetTimezone();
		},
		[](CUser* pUser, const CString& sVal, CString& sError) {
			pUser->SetTimezone(sVal);
			return true;
		},
		nullptr
	},
};

static const std::vector<Variable<CIRCNetwork>> NetworkVars = {
	{
		"AltNick", StringType,
		"An optional network specific alternate nick used if the primary nick is reserved.",
		[](const CIRCNetwork* pNetwork) {
			return pNetwork->GetAltNick();
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			pNetwork->SetAltNick(sVal);
			return true;
		},
		nullptr
	},
	{
		"BindHost", StringType,
		"An optional network specific bind host.",
		[](const CIRCNetwork* pNetwork) {
			return pNetwork->GetBindHost();
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			// TODO: move the sanity checking to CIRCNetwork::SetBindHost()
			if (pNetwork->GetUser()->DenySetBindHost() && !pNetwork->GetUser()->IsAdmin()) {
				sError = "Error: access denied!";
				return false;
			}

			if (sVal.Equals(pNetwork->GetBindHost())) {
				sError = "This bind host is already set!";
				return false;
			}

			const VCString& vsHosts = CZNC::Get().GetBindHosts();
			if (!pNetwork->GetUser()->IsAdmin() && !vsHosts.empty()) {
				bool bFound = false;

				for (const CString& vsHost : vsHosts) {
					if (sVal.Equals(vsHost)) {
						bFound = true;
						break;
					}
				}

				if (!bFound) {
					sError = "The bind host is not available. See /msg " + pNetwork->GetUser()->GetStatusPrefix() + "status ListBindHosts for the list of available bind hosts.";
					return false;
				}
			}

			pNetwork->SetBindHost(sVal);
			return true;
		},
		nullptr
	},
#ifdef HAVE_ICU
	{
		"Encoding", StringType,
		"An optional network specific client encoding.",
		[](const CIRCNetwork* pNetwork) {
			return pNetwork->GetEncoding();
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			pNetwork->SetEncoding(sVal);
			return true;
		},
		nullptr
	},
#endif
	{
		"FloodBurst", IntType,
		"The maximum amount of lines ZNC sends at once.",
		[](const CIRCNetwork* pNetwork) {
			return CString(pNetwork->GetFloodBurst());
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			pNetwork->SetFloodBurst(sVal.ToUShort());
			return true;
		},
		nullptr
	},
	{
		"FloodRate", DoubleType,
		"The number of lines per second ZNC sends after reaching the FloodBurst limit.",
		[](const CIRCNetwork* pNetwork) {
			return CString(pNetwork->GetFloodRate());
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			pNetwork->SetFloodRate(sVal.ToDouble());
			return true;
		},
		nullptr
	},
	{
		"Ident", StringType,
		"An optional network specific ident.",
		[](const CIRCNetwork* pNetwork) {
			return pNetwork->GetIdent();
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			pNetwork->SetIdent(sVal);
			return true;
		},
		nullptr
	},
	// TODO: IRCConnectEnabled?
	{
		"JoinDelay", IntType,
		"The delay in seconds, until channels are joined after getting connected.",
		[](const CIRCNetwork* pNetwork) {
			return CString(pNetwork->GetJoinDelay());
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			pNetwork->SetJoinDelay(sVal.ToUShort());
			return true;
		},
		nullptr
	},
	{
		"Nick", StringType,
		"An optional network specific primary nick.",
		[](const CIRCNetwork* pNetwork) {
			return pNetwork->GetNick();
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			pNetwork->SetNick(sVal);
			return true;
		},
		nullptr
	},
	{
		"QuitMsg", StringType,
		"An optional network specific quit message ZNC uses when disconnecting or shutting down.",
		[](const CIRCNetwork* pNetwork) {
			return pNetwork->GetQuitMsg();
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			pNetwork->SetQuitMsg(sVal);
			return true;
		},
		nullptr
	},
	{
		"RealName", StringType,
		"An optional network specific real name.",
		[](const CIRCNetwork* pNetwork) {
			return pNetwork->GetRealName();
		},
		[](CIRCNetwork* pNetwork, const CString& sVal, CString& sError) {
			pNetwork->SetRealName(sVal);
			return true;
		},
		nullptr
	},
};

static const std::vector<Variable<CChan>> ChanVars = {
	{
		"AutoClearChanBuffer", BoolType,
		"Whether the channel buffer is automatically cleared after playback.",
		[](const CChan* pChan) {
			CString sVal(pChan->AutoClearChanBuffer());
			if (!pChan->HasAutoClearChanBufferSet())
				sVal += " (default)";
			return sVal;
		},
		[](CChan* pChan, const CString& sVal, CString& sError) {
			if (sVal.Equals("-"))
				pChan->ResetAutoClearChanBuffer();
			else
				pChan->SetAutoClearChanBuffer(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"Buffer", IntType,
		"The maximum amount of lines stored for the channel specific playback buffer.",
		[](const CChan* pChan) {
			CString sVal(pChan->GetBufferCount());
			if (!pChan->HasBufferCountSet())
				sVal += " (default)";
			return sVal;
		},
		[](CChan* pChan, const CString& sVal, CString& sError) {
			unsigned int i = sVal.ToUInt();
			if (sVal.Equals("-"))
				pChan->ResetBufferCount();
			else if (!pChan->SetBufferCount(i, pChan->GetNetwork()->GetUser()->IsAdmin()))
				sError = "Setting failed, the limit is " + CString(CZNC::Get().GetMaxBufferSize());
			return sError.empty();
		},
		nullptr
	},
	{
		"Detached", BoolType,
		"Whether the channel is detached.",
		[](const CChan* pChan) {
			return CString(pChan->IsDetached());
		},
		[](CChan* pChan, const CString& sVal, CString& sError) {
			bool b = sVal.ToBool();
			if (b != pChan->IsDetached()) {
				if (b)
					pChan->DetachUser();
				else
					pChan->AttachUser();
			}
			return true;
		},
		nullptr
	},
	{
		"Disabled", BoolType,
		"Whether the channel is disabled.",
		[](const CChan* pChan) {
			return CString(pChan->IsDisabled());
		},
		[](CChan* pChan, const CString& sVal, CString& sError) {
			bool b = sVal.ToBool();
			if (b != pChan->IsDisabled()) {
				if (b)
					pChan->Disable();
				else
					pChan->Enable();
			}
			return true;
		},
		nullptr
	},
	{
		"InConfig", BoolType,
		"Whether the channel is stored in the config file.",
		[](const CChan* pChan) {
			return CString(pChan->InConfig());
		},
		[](CChan* pChan, const CString& sVal, CString& sError) {
			pChan->SetInConfig(sVal.ToBool());
			return true;
		},
		nullptr
	},
	{
		"Key", StringType,
		"An optional channel key.",
		[](const CChan* pChan) {
			return pChan->GetKey();
		},
		[](CChan* pChan, const CString& sVal, CString& sError) {
			pChan->SetKey(sVal);
			return true;
		},
		nullptr
	},
	{
		"Modes", StringType,
		"An optional set of default channel modes ZNC sets when joining an empty channel.",
		[](const CChan* pChan) {
			return pChan->GetDefaultModes();
		},
		[](CChan* pChan, const CString& sVal, CString& sError) {
			pChan->SetDefaultModes(sVal);
			return true;
		},
		nullptr
	},
};

static const std::vector<Variable<CSettingsMod>> ModVars = {
	{
		"Prefix", StringType,
		"A settings prefix (in addition to the status prefix) for settings queries.",
		[](const CSettingsMod* pMod) {
			return pMod->GetPrefix();
		},
		[](CSettingsMod* pMod, const CString& sVal, CString& sError) {
			pMod->SetPrefix(sVal);
			return true;
		},
		nullptr
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

	if (sCmd.Equals("Help")) {
		HandleHelpCommand(sLine);

		CString sPfx = GetUser()->GetStatusPrefix() + GetPrefix();
		CString sUsr = GetUser()->GetUserName();
		CString sNet = "network";
		CString sChan = "#chan";

		CIRCNetwork* pNetwork = GetNetwork();
		if (pNetwork) {
			sNet = pNetwork->GetName();
			const std::vector<CChan*> vChans = pNetwork->GetChans();
			if (!vChans.empty())
				sChan = vChans.front()->GetName();
		}

		if (sLine.Token(1).empty()) {
			PutModule("In order to adjust user, network and channel specific settings,");
			PutModule("open a query with <" + sPfx + "target>, where target is a user, network, or");
			PutModule("channel name.");
			PutModule("-----");
			PutModule("Examples:");
			PutModule("- user settings: /msg " + sPfx + sUsr + " help");
			PutModule("- network settings: /msg " + sPfx + sNet + " help");
			PutModule("- channel settings: /msg " + sPfx + sChan + " help");
			PutModule("-----");
			PutModule("To access network settings of a different user (admins only),");
			PutModule("or channel settings of a different network, the target can be");
			PutModule("a combination of a user, network, and channel name separated");
			PutModule("by a forward slash ('/') character.");
			PutModule("-----");
			PutModule("Advanced examples:");
			PutModule("- network settings of another user: /msg " + sPfx + "user/network help");
			PutModule("- channel settings of another network: /msg " + sPfx + "network/#chan help");
			PutModule("- channel settings of another network of another user: /msg " + sPfx + "user/network/#chan help");
			PutModule("-----");
		}
	} else if (sCmd.Equals("List")) {
		OnListCommand(this, GetModName(), sLine, ModVars);
	} else if (sCmd.Equals("Get")) {
		OnGetCommand(this, GetModName(), sLine, ModVars);
	} else if (sCmd.Equals("Set")) {
		OnSetCommand(this, GetModName(), sLine, ModVars);
	} else if (sCmd.Equals("Reset")) {
		OnResetCommand(this, GetModName(), sLine, ModVars);
	} else {
		PutModule("Unknown command!");
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
			// <global>
			if (sTgt.Equals("global"))
				return OnGlobalCommand(sPfx + sTgt, sRest);
			// <user>
			if (CUser* pUser = CZNC::Get().FindUser(sTgt))
				return OnUserCommand(pUser, sPfx + sTgt, sRest);
			// <network>
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
					PutLine(sPfx + sTgt, "Unknown (or ambiguous) network or channel!");
					return HALT;
				}
				// <network/#chan>
				if (CIRCNetwork* pNetwork = GetUser()->FindNetwork(vsParts[0])) {
					if (CChan* pChan = pNetwork->FindChan(vsParts[1])) {
						return OnChanCommand(pChan, sPfx + sTgt, sRest);
					} else {
						PutLine(sPfx + sTgt, "Unknown channel!");
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
							PutLine(sPfx + sTgt, "Unknown channel!");
							return HALT;
						}
					} else {
						PutLine(sPfx + sTgt, "Unknown network!");
						return HALT;
					}
				}
			}
		}
	}
	return CONTINUE;
}

CModule::EModRet CSettingsMod::OnGlobalCommand(const CString& sTgt, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (!GetUser()->IsAdmin() && sCmd.Equals("Set")) {
		PutLine(sTgt, "Error: access denied.");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sTgt, sLine, GlobalVars);
	else if (sCmd.Equals("List"))
		OnListCommand(&CZNC::Get(), sTgt, sLine, GlobalVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(&CZNC::Get(), sTgt, sLine, GlobalVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(&CZNC::Get(), sTgt, sLine, GlobalVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(&CZNC::Get(), sTgt, sLine, GlobalVars);
	else
		PutLine(sTgt, "Unknown command!");

	return HALT;
}

CModule::EModRet CSettingsMod::OnUserCommand(CUser* pUser, const CString& sTgt, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pUser != GetUser() && !GetUser()->IsAdmin()) {
		PutLine(sTgt, "Error: access denied.");
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
		PutLine(sTgt, "Unknown command!");

	return HALT;
}

CModule::EModRet CSettingsMod::OnNetworkCommand(CIRCNetwork* pNetwork, const CString& sTgt, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pNetwork->GetUser() != GetUser() && !GetUser()->IsAdmin()) {
		PutLine(sTgt, "Error: access denied.");
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
		PutLine(sTgt, "Unknown command!");

	return HALT;
}

CModule::EModRet CSettingsMod::OnChanCommand(CChan* pChan, const CString& sTgt, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pChan->GetNetwork()->GetUser() != GetUser() && !GetUser()->IsAdmin()) {
		PutLine(sTgt, "Error: access denied.");
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
		PutLine(sTgt, "Unknown command!");

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
		PutLine(sTgt, "Unknown command!");
}

template <typename T, typename V>
void CSettingsMod::OnListCommand(T* pTarget, const CString& sTgt, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sFilter = sLine.Token(1);

	const CTable Table = FilterVarTable(vVars, sFilter);
	if (!Table.empty())
		PutTable(sTgt, Table);
	else
		PutLine(sTgt, "Unknown variable!");
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
		PutLine(sTgt, "Unknown variable!");
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
			if (!Var.setter(pTarget, sVal, sError)) {
				PutLine(sTgt, sError);
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
		PutLine(sTgt, "Unknown variable!");
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
				PutLine(sTgt, "Error: reset not supported!");
			} else if (!Var.resetter(pTarget, sError)) {
				PutLine(sTgt, sError);
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
		PutLine(sTgt, "Error: unknown variable!");
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

USERMODULEDEFS(CSettingsMod, "Adjust your settings conveniently through IRC.")
