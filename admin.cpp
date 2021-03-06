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
#include <znc/IRCNetwork.h>
#include <znc/Listener.h>
#include <znc/IRCSock.h>
#include <znc/Server.h>
#include <znc/User.h>
#include <znc/Chan.h>
#include <znc/znc.h>
#include <functional>

#if (VERSION_MAJOR < 1) || (VERSION_MAJOR == 1 && VERSION_MINOR < 7)
#error The admin module requires ZNC version 1.7.0 or later.
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
	std::function<CString(const T*)> get;
	std::function<bool(T*, const CString&)> set;
	std::function<bool(T*)> reset;
};

template <typename T>
struct Command
{
	CString syntax;
	CString description;
	std::function<void(T*, const CString&)> exec;
};

class CAdminMod : public CModule
{
public:
	MODCONSTRUCTOR(CAdminMod)
	{
	}

	void OnModCommand(const CString& sLine) override;
	EModRet OnUserRaw(CString& sLine) override;

	CString GetInfix() const;
	void SetInfix(const CString& sInfix);

protected:
	EModRet OnUserCommand(CUser* pUser, const CString& sLine);
	EModRet OnNetworkCommand(CIRCNetwork* pNetwork, const CString& sLine);
	EModRet OnChanCommand(CChan* pChan, const CString& sLine);

private:
	template <typename C>
	void OnHelpCommand(const CString& sLine, const std::vector<C>& vCmds);
	template <typename T, typename V>
	void OnListCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnGetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnSetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnResetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename C>
	void OnExecCommand(T* pObject, const CString& sLine, const std::vector<C>& vCmds);

	template <typename T>
	void OnListModsCommand(T* pObject, const CString& sArgs, CModInfo::EModuleType eType);
	template <typename T>
	void OnLoadModCommand(T* pObject, const CString& sArgs, CModInfo::EModuleType eType);
	template <typename T>
	void OnReloadModCommand(T* pObject, const CString& sArgs);
	template <typename T>
	void OnUnloadModCommand(T* pObject, const CString& sArgs);

	template <typename C>
	CTable FilterCmdTable(const std::vector<C>& vCmds, const CString& sFilter) const;
	template <typename V>
	CTable FilterVarTable(const std::vector<V>& vVars, const CString& sFilter) const;

	void PutSuccess(const CString& sLine, const CString& sTarget = "");
	void PutUsage(const CString& sSyntax, const CString& sTarget = "");
	void PutError(const CString& sLine, const CString& sTarget = "");
	void PutLine(const CString& sLine, const CString& sTarget = "");
	void PutTable(const CTable& Table, const CString& sTarget = "");

	CString m_sTarget;


	// TODO: expose the default constants needed by the reset methods?

	const std::vector<Variable<CZNC>> GlobalVars = {
		{
			"AnonIPLimit", IntType,
			"The limit of anonymous unidentified connections per IP.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetAnonIPLimit());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetAnonIPLimit(sVal.ToUInt());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetAnonIPLimit(10);
				return true;
			}
		},
		// TODO: BindHost
		{
			"ConnectDelay", IntType,
			"The number of seconds every IRC connection is delayed.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetConnectDelay());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetConnectDelay(sVal.ToUInt());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetConnectDelay(5);
				return true;
			}
		},
		{
			"HideVersion", BoolType,
			"Whether the version number is hidden from the web interface and CTCP VERSION replies.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetHideVersion());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetHideVersion(sVal.ToBool());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetHideVersion(false);
				return true;
			}
		},
		{
			"MaxBufferSize", IntType,
			"The maximum playback buffer size. Only admin users can exceed the limit.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetMaxBufferSize());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetMaxBufferSize(sVal.ToUInt());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetMaxBufferSize(500);
				return true;
			}
		},
		{
			"Motd", ListType,
			"The list of 'message of the day' lines that are sent to clients on connect via notice from *status.",
			[=](const CZNC* pZNC) {
				const VCString& vsMotd = pZNC->GetMotd();
				return CString("\n").Join(vsMotd.begin(), vsMotd.end());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->AddMotd(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->ClearMotd();
				return true;
			},
		},
		// TODO: PidFile
		{
			"ProtectWebSessions", BoolType,
			"Whether IP changing during each web session is disallowed.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetProtectWebSessions());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetProtectWebSessions(sVal.ToBool());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetProtectWebSessions(true);
				return true;
			}
		},
		{
			"ServerThrottle", IntType,
			"The number of seconds between connect attempts to the same hostname.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetServerThrottle());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetServerThrottle(sVal.ToUInt());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetServerThrottle(30);
				return true;
			}
		},
		{
			"Skin", StringType,
			"The default web interface skin.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSkinName();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetSkinName(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetSkinName("");
				return true;
			}
		},
		{
			"SSLCertFile", StringType,
			"The TLS/SSL certificate file from which ZNC reads its server certificate.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSSLCertFile();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetSSLCertFile(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetSSLCertFile(pZNC->GetZNCPath() + "/znc.pem");
				return true;
			},
		},
		{
			"SSLCiphers", StringType,
			"The allowed SSL ciphers. Default value is from Mozilla's recomendations.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSSLCiphers();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetSSLCiphers(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetSSLCiphers("");
				return true;
			},
		},
		{
			"SSLProtocols", StringType,
			"The accepted SSL protocols.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSSLProtocols();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				if (!pZNC->SetSSLProtocols(sVal)) {
					VCString vsProtocols = pZNC->GetAvailableSSLProtocols();
					PutError("invalid value");
					PutError("the syntax is: [+|-]<protocol> ...");
					PutError("available protocols: " + CString(", ").Join(vsProtocols.begin(), vsProtocols.end()));
					return false;
				}
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetSSLProtocols("");
				return true;
			},
		},
		{
			"StatusPrefix", StringType,
			"The default prefix for status and module queries.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSkinName();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetSkinName(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetStatusPrefix("");
				return true;
			}
		},
		{
			"TrustedProxy", ListType,
			"The list of trusted proxies.",
			[=](const CZNC* pZNC) {
				const VCString& vsProxies = pZNC->GetTrustedProxies();
				return CString("\n").Join(vsProxies.begin(), vsProxies.end());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				SCString ssProxies;
				sVal.Split(" ", ssProxies, false);
				for (const CString& sProxy : ssProxies)
					pZNC->AddTrustedProxy(sProxy);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->ClearTrustedProxies();
				return true;
			}
		},
	};

	const std::vector<Variable<CUser>> UserVars = {
		{
			"Admin", BoolType,
			"Whether the user has admin rights.",
			[=](const CUser* pUser) {
				return CString(pUser->IsAdmin());
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetAdmin(sVal.ToBool());
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetAdmin(false);
				return true;
			}
		},
		{
			"AdminInfix", StringType,
			"An infix (after the status prefix) for admin queries.",
			[=](const CUser* pUser) {
				return GetInfix();
			},
			[=](CUser* pUser, const CString& sVal) {
				SetInfix(sVal);
				return true;
			},
			[=](CUser* pUser) {
				SetInfix(GetUser()->GetStatusPrefix());
				return true;
			}
		},
		{
			"Allow", ListType,
			"The list of allowed IPs for the user. Wildcards (*) are supported.",
			[=](const CUser* pUser) {
				const SCString& ssHosts = pUser->GetAllowedHosts();
				return CString("\n").Join(ssHosts.begin(), ssHosts.end());
			},
			[=](CUser* pUser, const CString& sVal) {
				SCString ssHosts;
				sVal.Split(" ", ssHosts, false);
				for (const CString& sHost : ssHosts)
					pUser->AddAllowedHost(sHost);
				return true;
			},
			[=](CUser* pUser) {
				pUser->ClearAllowedHosts();
				return true;
			}
		},
		{
			"AltNick", StringType,
			"The default alternate nick.",
			[=](const CUser* pUser) {
				return pUser->GetAltNick();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetAltNick(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetAltNick("");
				return true;
			}
		},
		{
			"AppendTimestamp", BoolType,
			"Whether timestamps are appended to buffer playback messages.",
			[=](const CUser* pUser) {
				return CString(pUser->GetTimestampAppend());
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetTimestampAppend(sVal.ToBool());
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetTimestampAppend(false);
				return true;
			}
		},
		{
			"AutoClearChanBuffer", BoolType,
			"Whether channel buffers are automatically cleared after playback.",
			[=](const CUser* pUser) {
				return CString(pUser->AutoClearChanBuffer());
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetAutoClearChanBuffer(sVal.ToBool());
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetAutoClearChanBuffer(true);
				return true;
			}
		},
		{
			"AutoClearQueryBuffer", BoolType,
			"Whether query buffers are automatically cleared after playback.",
			[=](const CUser* pUser) {
				return CString(pUser->AutoClearQueryBuffer());
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetAutoClearQueryBuffer(sVal.ToBool());
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetAutoClearQueryBuffer(true);
				return true;
			}
		},
		{
			"BindHost", StringType,
			"The default bind host.",
			[=](const CUser* pUser) {
				return pUser->GetBindHost();
			},
			[=](CUser* pUser, const CString& sVal) {
				if (!GetUser()->IsAdmin() && GetUser()->DenySetBindHost()) {
					PutError("access denied");
					return false;
				}
				pUser->SetBindHost(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetBindHost("");
				return true;
			}
		},
		{
			"ChanBufferSize", IntType,
			"The maximum amount of lines stored for each channel playback buffer.",
			[=](const CUser* pUser) {
				return CString(pUser->GetChanBufferSize());
			},
			[=](CUser* pUser, const CString& sVal) {
				if (!pUser->SetChanBufferSize(sVal.ToUInt(), GetUser()->IsAdmin())) {
					PutError("exceeded limit " + CString(CZNC::Get().GetMaxBufferSize()));
					return false;
				}
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetChanBufferSize(50);
				return true;
			},
		},
		{
			"ChanModes", StringType,
			"The default modes ZNC sets when joining an empty channel.",
			[=](const CUser* pUser) {
				return pUser->GetDefaultChanModes();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetDefaultChanModes(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetDefaultChanModes("");
				return true;
			}
		},
	#ifdef HAVE_ICU
		{
			"ClientEncoding", StringType,
			"The default client encoding.",
			[=](const CUser* pUser) {
				return pUser->GetClientEncoding();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetClientEncoding(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetClientEncoding("");
				return true;
			}
		},
	#endif
		{
			"CTCPReply", ListType,
			"A list of CTCP request-reply-pairs. Syntax: <request> <reply>.",
			[=](const CUser* pUser) {
				VCString vsReplies;
				for (const auto& it : pUser->GetCTCPReplies())
					vsReplies.push_back(it.first + " " + it.second);
				return CString("\n").Join(vsReplies.begin(), vsReplies.end());
			},
			[=](CUser* pUser, const CString& sVal) {
				CString sRequest = sVal.Token(0);
				CString sReply = sVal.Token(1, true);
				if (sReply.empty()) {
					if (!pUser->DelCTCPReply(sRequest.AsUpper())) {
						PutError("unable to remove");
						return false;
					}
				} else {
					if (!pUser->AddCTCPReply(sRequest, sReply)) {
						PutError("unable to add");
						return false;
					}
				}
				return true;
			},
			[=](CUser* pUser) {
				MCString mReplies = pUser->GetCTCPReplies();
				for (const auto& it : mReplies)
					pUser->DelCTCPReply(it.first);
				return true;
			}
		},
		{
			"DCCBindHost", StringType,
			"An optional bindhost for DCC connections.",
			[=](const CUser* pUser) {
				return pUser->GetDCCBindHost();
			},
			[=](CUser* pUser, const CString& sVal) {
				if (!GetUser()->IsAdmin() && GetUser()->DenySetBindHost()) {
					PutError("access denied");
					return false;
				}
				pUser->SetDCCBindHost(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetDCCBindHost("");
				return true;
			}
		},
		{
			"DenyLoadMod", BoolType,
			"Whether the user is denied access to load modules.",
			[=](const CUser* pUser) {
				return CString(pUser->DenyLoadMod());
			},
			[=](CUser* pUser, const CString& sVal) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pUser->SetDenyLoadMod(sVal.ToBool());
				return true;
			},
			[=](CUser* pUser) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pUser->SetDenyLoadMod(false);
				return true;
			}
		},
		{
			"DenySetBindHost", BoolType,
			"Whether the user is denied access to set a bind host.",
			[=](const CUser* pUser) {
				return CString(pUser->DenySetBindHost());
			},
			[=](CUser* pUser, const CString& sVal) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pUser->SetDenySetBindHost(sVal.ToBool());
				return true;
			},
			[=](CUser* pUser) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pUser->SetDenySetBindHost(false);
				return true;
			}
		},
		{
			"Ident", StringType,
			"The default ident.",
			[=](const CUser* pUser) {
				return pUser->GetIdent();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetIdent(sVal);
				return true;
			},
			nullptr
		},
		{
			"JoinTries", IntType,
			"The amount of times channels are attempted to join in case of a failure.",
			[=](const CUser* pUser) {
				return CString(pUser->JoinTries());
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetJoinTries(sVal.ToUInt());
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetJoinTries(10);
				return true;
			}
		},
		{
			"MaxJoins", IntType,
			"The maximum number of channels ZNC joins at once.",
			[=](const CUser* pUser) {
				return CString(pUser->MaxJoins());
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetMaxJoins(sVal.ToUInt());
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetMaxJoins(0);
				return true;
			}
		},
		{
			"MaxNetworks", IntType,
			"The maximum number of networks the user is allowed to have.",
			[=](const CUser* pUser) {
				return CString(pUser->MaxNetworks());
			},
			[=](CUser* pUser, const CString& sVal) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pUser->SetMaxNetworks(sVal.ToUInt());
				return true;
			},
			[=](CUser* pUser) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pUser->SetMaxNetworks(1);
				return true;
			}
		},
		{
			"MaxQueryBuffers", IntType,
			"The maximum number of query buffers that are stored.",
			[=](const CUser* pUser) {
				return CString(pUser->MaxQueryBuffers());
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetMaxQueryBuffers(sVal.ToUInt());
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetMaxQueryBuffers(50);
				return true;
			}
		},
		{
			"MultiClients", BoolType,
			"Whether multiple clients are allowed to connect simultaneously.",
			[=](const CUser* pUser) {
				return CString(pUser->MultiClients());
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetMultiClients(sVal.ToBool());
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetMultiClients(true);
				return true;
			}
		},
		{
			"Nick", StringType,
			"The default primary nick.",
			[=](const CUser* pUser) {
				return pUser->GetNick();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetNick(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetNick("");
				return true;
			}
		},
		{
			"PrependTimestamp", BoolType,
			"Whether timestamps are prepended to buffer playback messages.",
			[=](const CUser* pUser) {
				return CString(pUser->GetTimestampPrepend());
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetTimestampPrepend(sVal.ToBool());
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetTimestampPrepend(true);
				return true;
			}
		},
		{
			"Password", StringType,
			"",
			[=](const CUser* pUser) {
				return CString(".", pUser->GetPass().size());
			},
			[=](CUser* pUser, const CString& sVal) {
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
			[=](const CUser* pUser) {
				return CString(pUser->GetQueryBufferSize());
			},
			[=](CUser* pUser, const CString& sVal) {
				if (!pUser->SetQueryBufferSize(sVal.ToUInt(), GetUser()->IsAdmin())) {
					PutError("exceeded limit " + CString(CZNC::Get().GetMaxBufferSize()));
					return false;
				}
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetQueryBufferSize(50);
				return true;
			}
		},
		{
			"QuitMsg", StringType,
			"The default quit message ZNC uses when disconnecting or shutting down.",
			[=](const CUser* pUser) {
				return pUser->GetQuitMsg();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetQuitMsg(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetQuitMsg("");
				return true;
			}
		},
		{
			"RealName", StringType,
			"The default real name.",
			[=](const CUser* pUser) {
				return pUser->GetRealName();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetRealName(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetRealName("");
				return true;
			}
		},
		{
			"Skin", StringType,
			"The web interface skin.",
			[=](const CUser* pUser) {
				return pUser->GetSkinName();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetSkinName(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetSkinName("");
				return true;
			}
		},
		{
			"StatusPrefix", StringType,
			"The prefix for status and module queries.",
			[=](const CUser* pUser) {
				return pUser->GetStatusPrefix();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetStatusPrefix(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetStatusPrefix("*");
				return true;
			}
		},
		{
			"TimestampFormat", StringType,
			"The format of the timestamps used in buffer playback messages.",
			[=](const CUser* pUser) {
				return pUser->GetTimestampFormat();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetTimestampFormat(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetTimestampFormat("[%H:%M:%S]");
				return true;
			}
		},
		{
			"Timezone", StringType,
			"The timezone used for timestamps in buffer playback messages.",
			[=](const CUser* pUser) {
				return pUser->GetTimezone();
			},
			[=](CUser* pUser, const CString& sVal) {
				pUser->SetTimezone(sVal);
				return true;
			},
			[=](CUser* pUser) {
				pUser->SetTimezone("");
				return true;
			}
		},
	};

	const std::vector<Variable<CIRCNetwork>> NetworkVars = {
		{
			"AltNick", StringType,
			"An optional network specific alternate nick used if the primary nick is reserved.",
			[=](const CIRCNetwork* pNetwork) {
				return pNetwork->GetAltNick();
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->SetAltNick(sVal);
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetAltNick("");
				return true;
			}
		},
		{
			"BindHost", StringType,
			"An optional network specific bind host.",
			[=](const CIRCNetwork* pNetwork) {
				return pNetwork->GetBindHost();
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				if (!GetUser()->IsAdmin() && GetUser()->DenySetBindHost()) {
					PutError("access denied");
					return false;
				}
				pNetwork->SetBindHost(sVal);
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetBindHost("");
				return true;
			}
		},
	#ifdef HAVE_ICU
		{
			"Encoding", StringType,
			"An optional network specific client encoding.",
			[=](const CIRCNetwork* pNetwork) {
				return pNetwork->GetEncoding();
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->SetEncoding(sVal);
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetEncoding("");
				return true;
			}
		},
	#endif
		{
			"FloodBurst", IntType,
			"The maximum amount of lines ZNC sends at once.",
			[=](const CIRCNetwork* pNetwork) {
				return CString(pNetwork->GetFloodBurst());
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->SetFloodBurst(sVal.ToUShort());
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetFloodBurst(4);
				return true;
			}
		},
		{
			"FloodRate", DoubleType,
			"The number of lines per second ZNC sends after reaching the FloodBurst limit.",
			[=](const CIRCNetwork* pNetwork) {
				return CString(pNetwork->GetFloodRate());
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->SetFloodRate(sVal.ToDouble());
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetFloodRate(1);
				return true;
			}
		},
		{
			"Ident", StringType,
			"An optional network specific ident.",
			[=](const CIRCNetwork* pNetwork) {
				return pNetwork->GetIdent();
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->SetIdent(sVal);
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetIdent("");
				return true;
			}
		},
		// TODO: IRCConnectEnabled?
		{
			"JoinDelay", IntType,
			"The delay in seconds, until channels are joined after getting connected.",
			[=](const CIRCNetwork* pNetwork) {
				return CString(pNetwork->GetJoinDelay());
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->SetJoinDelay(sVal.ToUShort());
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetJoinDelay(0);
				return true;
			}
		},
		{
			"Nick", StringType,
			"An optional network specific primary nick.",
			[=](const CIRCNetwork* pNetwork) {
				return pNetwork->GetNick();
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->SetNick(sVal);
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetNick("");
				return true;
			}
		},
		{
			"QuitMsg", StringType,
			"An optional network specific quit message ZNC uses when disconnecting or shutting down.",
			[=](const CIRCNetwork* pNetwork) {
				return pNetwork->GetQuitMsg();
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->SetQuitMsg(sVal);
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetQuitMsg("");
				return true;
			}
		},
		{
			"RealName", StringType,
			"An optional network specific real name.",
			[=](const CIRCNetwork* pNetwork) {
				return pNetwork->GetRealName();
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->SetRealName(sVal);
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->SetRealName("");
				return true;
			}
		},
		{
			"TrustedServerFingerprint", ListType,
			"The list of trusted server fingerprints.",
			[=](const CIRCNetwork* pNetwork) {
				const SCString& sFP = pNetwork->GetTrustedFingerprints();
				return CString("\n").Join(sFP.begin(), sFP.end());
			},
			[=](CIRCNetwork* pNetwork, const CString& sVal) {
				pNetwork->AddTrustedFingerprint(sVal);
				return true;
			},
			[=](CIRCNetwork* pNetwork) {
				pNetwork->ClearTrustedFingerprints();
				return true;
			}
		},
	};

	const std::vector<Variable<CChan>> ChanVars = {
		{
			"AutoClearChanBuffer", BoolType,
			"Whether the channel buffer is automatically cleared after playback.",
			[=](const CChan* pChan) {
				CString sVal(pChan->AutoClearChanBuffer());
				if (!pChan->HasAutoClearChanBufferSet())
					sVal += " (default)";
				return sVal;
			},
			[=](CChan* pChan, const CString& sVal) {
				pChan->SetAutoClearChanBuffer(sVal.ToBool());
				return true;
			},
			[=](CChan* pChan) {
				pChan->ResetAutoClearChanBuffer();
				return true;
			}
		},
		{
			"Buffer", IntType,
			"The maximum amount of lines stored for the channel specific playback buffer.",
			[=](const CChan* pChan) {
				CString sVal(pChan->GetBufferCount());
				if (!pChan->HasBufferCountSet())
					sVal += " (default)";
				return sVal;
			},
			[=](CChan* pChan, const CString& sVal) {
				if (!pChan->SetBufferCount(sVal.ToUInt(), GetUser()->IsAdmin())) {
					PutError("exceeded limit " + CString(CZNC::Get().GetMaxBufferSize()));
					return false;
				}
				return true;
			},
			[=](CChan* pChan) {
				pChan->ResetBufferCount();
				return true;
			}
		},
		{
			"Detached", BoolType,
			"Whether the channel is detached.",
			[=](const CChan* pChan) {
				return CString(pChan->IsDetached());
			},
			[=](CChan* pChan, const CString& sVal) {
				bool b = sVal.ToBool();
				if (b != pChan->IsDetached()) {
					if (b)
						pChan->DetachUser();
					else
						pChan->AttachUser();
				}
				return true;
			},
			[=](CChan* pChan) {
				if (pChan->IsDetached())
					pChan->AttachUser();
				return true;
			}
		},
		{
			"Disabled", BoolType,
			"Whether the channel is disabled.",
			[=](const CChan* pChan) {
				return CString(pChan->IsDisabled());
			},
			[=](CChan* pChan, const CString& sVal) {
				bool b = sVal.ToBool();
				if (b != pChan->IsDisabled()) {
					if (b)
						pChan->Disable();
					else
						pChan->Enable();
				}
				return true;
			},
			[=](CChan* pChan) {
				if (pChan->IsDisabled())
					pChan->Enable();
				return true;
			}
		},
		{
			"InConfig", BoolType,
			"Whether the channel is stored in the config file.",
			[=](const CChan* pChan) {
				return CString(pChan->InConfig());
			},
			[=](CChan* pChan, const CString& sVal) {
				pChan->SetInConfig(sVal.ToBool());
				return true;
			},
			nullptr
		},
		{
			"Key", StringType,
			"An optional channel key.",
			[=](const CChan* pChan) {
				return pChan->GetKey();
			},
			[=](CChan* pChan, const CString& sVal) {
				pChan->SetKey(sVal);
				return true;
			},
			[=](CChan* pChan) {
				pChan->SetKey("");
				return true;
			}
		},
		{
			"Modes", StringType,
			"An optional set of default channel modes ZNC sets when joining an empty channel.",
			[=](const CChan* pChan) {
				return pChan->GetDefaultModes();
			},
			[=](CChan* pChan, const CString& sVal) {
				pChan->SetDefaultModes(sVal);
				return true;
			},
			[=](CChan* pChan) {
				pChan->SetDefaultModes("");
				return true;
			}
		},
	};

	const std::vector<Command<CZNC>> GlobalCmds = {
		{
			"AddPort <[+]port> <ipv4|ipv6|all> <web|irc|all> [bindhost [uriprefix]]",
			"Adds a port for ZNC to listen on.",
			[=](CZNC* pZNC, const CString& sArgs) {
				CString sPort = sArgs.Token(0);
				CString sAddr = sArgs.Token(1);
				CString sAccept = sArgs.Token(2);
				CString sBindHost = sArgs.Token(3);
				CString sURIPrefix = sArgs.Token(4);

				unsigned short uPort = sPort.ToUShort();
				bool bSSL = sPort.StartsWith("+");

				EAddrType eAddr = ADDR_ALL;
				if (sAddr.Equals("IPV4"))
					eAddr = ADDR_IPV4ONLY;
				else if (sAddr.Equals("IPV6"))
					eAddr = ADDR_IPV6ONLY;
				else if (sAddr.Equals("ALL"))
					eAddr = ADDR_ALL;
				else
					sAddr.clear();

				CListener::EAcceptType eAccept = CListener::ACCEPT_ALL;
				if (sAccept.Equals("WEB"))
					eAccept = CListener::ACCEPT_HTTP;
				else if (sAccept.Equals("IRC"))
					eAccept = CListener::ACCEPT_IRC;
				else if (sAccept.Equals("ALL"))
					eAccept = CListener::ACCEPT_ALL;
				else
					sAccept.clear();

				if (sPort.empty() || sAddr.empty() || sAccept.empty()) {
					PutUsage("AddPort <[+]port> <ipv4|ipv6|all> <web|irc|all> [bindhost [uriprefix]]");
					return;
				}

				CListener* pListener = new CListener(uPort, sBindHost, sURIPrefix, bSSL, eAddr, eAccept);
				if (!pListener->Listen()) {
					delete pListener;
					PutError("unable to bind '" + CString(strerror(errno)) + "'");
				} else {
					CString sError;
					if (!pZNC->AddListener(pListener))
						PutError("internal error");
					else
						PutSuccess("port added");
				}
			}
		},
		{
			"AddUser <username> <password>",
			"Adds a new user.",
			[=](CZNC* pZNC, const CString& sArgs) {
				const CString sUsername = sArgs.Token(0);
				const CString sPassword = sArgs.Token(1);
				if (sPassword.empty()) {
					PutUsage("AddUser <username> <password>");
					return;
				}

				CUser* pUser = pZNC->FindUser(sUsername);
				if (pUser) {
					PutError("user '" + pUser->GetUserName() + "' already exists");
					return;
				}

				pUser = new CUser(sUsername);
				CString sSalt = CUtils::GetSalt();
				pUser->SetPass(CUser::SaltedHash(sPassword, sSalt), CUser::HASH_DEFAULT, sSalt);

				CString sError;
				if (pZNC->AddUser(pUser, sError)) {
					PutSuccess("user '" + pUser->GetUserName() + "' added");
				} else {
					PutError(sError);
					delete pUser;
				}
			}
		},
		{
			"Broadcast <message>",
			"Broadcasts a message to all ZNC users.",
			[=](CZNC* pZNC, const CString& sArgs) {
				if (sArgs.empty()) {
					PutUsage("Broadcast <message>");
					return;
				}
				pZNC->Broadcast(sArgs);
			}
		},
		{
			"DelPort <[+]port> <ipv4|ipv6|all> [bindhost]",
			"Deletes a port.",
			[=](CZNC* pZNC, const CString& sArgs) {
				CString sPort = sArgs.Token(0);
				CString sAddr = sArgs.Token(1);
				CString sBindHost = sArgs.Token(2);

				unsigned short uPort = sPort.ToUShort();

				EAddrType eAddr = ADDR_ALL;
				if (sAddr.Equals("IPV4"))
					eAddr = ADDR_IPV4ONLY;
				else if (sAddr.Equals("IPV6"))
					eAddr = ADDR_IPV6ONLY;
				else if (sAddr.Equals("ALL"))
					eAddr = ADDR_ALL;
				else
					sAddr.clear();

				if (sPort.empty() || sAddr.empty()) {
					PutUsage("DelPort <port> <ipv4|ipv6|all> [bindhost]");
					return;
				}

				CListener* pListener = pZNC->FindListener(uPort, sBindHost, eAddr);
				if (pListener) {
					pZNC->DelListener(pListener);
					PutSuccess("port deleted");
				} else {
					PutError("no matching port");
				}
			}
		},
		{
			"DelUser <username>",
			"Deletes a user.",
			[=](CZNC* pZNC, const CString& sArgs) {
				const CString sUsername = sArgs.Token(0);
				if (sUsername.empty()) {
					PutUsage("DelUser <username>");
					return;
				}

				CUser *pUser = pZNC->FindUser(sUsername);
				if (!pUser) {
					PutError("user '" + sUsername + "' doesn't exist");
					return;
				}

				if (pUser == GetUser()) {
					PutError("access denied");
					return;
				}

				if (pZNC->DeleteUser(pUser->GetUserName()))
					PutSuccess("user '" + pUser->GetUserName() + "' deleted");
				else
					PutError("internal error");
			}
		},
		{
			"ListMods [filter]",
			"Lists global modules.",
			[=](CZNC* pZNC, const CString& sArgs) {
				OnListModsCommand(pZNC, sArgs, CModInfo::GlobalModule);
			}
		},
		{
			"ListUsers [filter]",
			"Lists all ZNC users.",
			[=](CZNC* pZNC, const CString& sArgs) {
				CTable Table;
				Table.AddColumn("Username");
				Table.AddColumn("Networks");
				Table.AddColumn("Clients");

				for (const auto& it : pZNC->GetUserMap()) {
					Table.AddRow();
					Table.SetCell("Username", it.first);
					Table.SetCell("Networks", CString(it.second->GetNetworks().size()));
					Table.SetCell("Clients", CString(it.second->GetAllClients().size()));
				}

				PutTable(Table);
			}
		},
		{
			"ListPorts [filter]",
			"Lists all ZNC ports.",
			[=](CZNC* pZNC, const CString& sArgs) {
				const CString& sFilter = sArgs.Token(0);

				CTable Table;
				Table.AddColumn("Port");
				Table.AddColumn("Options");

				for (const CListener* pListener : pZNC->GetListeners()) {
					VCString vsOptions;
					vsOptions.push_back(pListener->GetBindHost().empty() ? "*" : pListener->GetBindHost());
					if (pListener->GetAddrType() == ADDR_IPV6ONLY || pListener->GetAddrType() == ADDR_ALL)
						vsOptions.push_back("IPv6");
					if (pListener->GetAddrType() == ADDR_IPV4ONLY || pListener->GetAddrType() == ADDR_ALL)
						vsOptions.push_back("IPv4");
					if (pListener->GetAcceptType() == CListener::ACCEPT_ALL || pListener->GetAcceptType() == CListener::ACCEPT_IRC)
						vsOptions.push_back("IRC");
					if (pListener->GetAcceptType() == CListener::ACCEPT_ALL || pListener->GetAcceptType() == CListener::ACCEPT_HTTP) {
						vsOptions.push_back("WEB");
						if (!pListener->GetURIPrefix().empty())
							vsOptions.push_back(pListener->GetURIPrefix() + "/");
					}

					if (!sFilter.empty()) {
						bool bMatch = false;

						if (CString(pListener->GetPort()).WildCmp(sFilter.TrimPrefix_n("+"))) {
							bMatch = true;
						} else {
							for (const CString& sOption : vsOptions) {
								if (sOption.Equals(sFilter)) {
									bMatch = true;
									break;
								}
							}
						}

						if (!bMatch) {
							PutLine("No matches for '" + sFilter + "'");
							return;
						}
					}

					Table.AddRow();
					if (pListener->IsSSL())
						Table.SetCell("Port", "+" + CString(pListener->GetPort()));
					else
						Table.SetCell("Port", CString(pListener->GetPort()));
					Table.SetCell("Options", CString(", ").Join(vsOptions.begin(), vsOptions.end()));
				}

				PutTable(Table);
			}
		},
		{
			"LoadMod <module> [args]",
			"Loads a global module.",
			[=](CZNC* pZNC, const CString& sArgs) {
				OnLoadModCommand(pZNC, sArgs, CModInfo::GlobalModule);
			}
		},
		{
			"Rehash",
			"Reloads the ZNC configuration file.",
			[=](CZNC* pZNC, const CString& sArgs) {
				CString sError;
				if (pZNC->RehashConfig(sError))
					PutError("failed to read '" + pZNC->GetConfigFile() + "'");
				else
					PutSuccess("read '" + pZNC->GetConfigFile() + "'");
			}
		},
		{
			"ReloadMod <module> [args]",
			"Reloads a global module.",
			[=](CZNC* pZNC, const CString& sArgs) {
				OnReloadModCommand(pZNC, sArgs);
			}
		},
		{
			"Restart [--force] [message]",
			"Restarts ZNC.",
			[=](CZNC* pZNC, const CString& sArgs) {
				bool bForce = sArgs.Token(0).Equals("--force");
				CString sMessage = sArgs.Token(bForce ? 1 : 0, true);
				if (sMessage.empty())
					sMessage = "ZNC is being restarted NOW!";

				if (!pZNC->WriteConfig() && !bForce) {
					PutError("saving config failed");
					PutLine("Aborting. Use --force to ignore.");
				} else {
					pZNC->Broadcast(sMessage);
					throw CException(CException::EX_Restart);
				}
			}
		},
		{
			"SaveConfig",
			"Saves the ZNC configuration file.",
			[=](CZNC* pZNC, const CString& sArgs) {
				if (!pZNC->WriteConfig())
					PutError("failed to write '" + pZNC->GetConfigFile() + "'");
				else
					PutSuccess("wrote '" + pZNC->GetConfigFile() + "'");
			}
		},
		{
			"Shutdown [--force] [message]",
			"Shuts down ZNC.",
			[=](CZNC* pZNC, const CString& sArgs) {
				bool bForce = sArgs.Token(0).Equals("--force");
				CString sMessage = sArgs.Token(bForce ? 1 : 0, true);
				if (sMessage.empty())
					sMessage = "ZNC is being shut down NOW!";

				if (!pZNC->WriteConfig() && !bForce) {
					PutError("saving config failed");
					PutLine("Aborting. Use --force to ignore.");
				} else {
					pZNC->Broadcast(sMessage);
					throw CException(CException::EX_Shutdown);
				}
			}
		},
		{
			"Traffic",
			"Shows the amount of traffic.",
			[=](CZNC* pZNC, const CString& sArgs) {
				CTable Table;
				Table.AddColumn("User");
				Table.AddColumn("Sent");
				Table.AddColumn("Received");
				Table.AddColumn("Total");
				for (const auto& it : pZNC->GetUserMap()) {
					Table.AddRow();
					Table.SetCell("User", it.first);
					Table.SetCell("Sent", CString::ToByteStr(it.second->BytesWritten()));
					Table.SetCell("Received", CString::ToByteStr(it.second->BytesRead()));
					Table.SetCell("Total", CString::ToByteStr(it.second->BytesRead() + it.second->BytesWritten()));
				}
				PutTable(Table);
			}
		},
		{
			"UnloadMod <module> [args]",
			"Unloads a global module.",
			[=](CZNC* pZNC, const CString& sArgs) {
				OnUnloadModCommand(pZNC, sArgs);
			}
		},
		{
			"UpdateMod <module>",
			"Reloads all instances of a module.",
			[=](CZNC* pZNC, const CString& sArgs) {
				const CString sMod = sArgs.Token(0);
				if (sMod.empty()) {
					PutUsage("UpdateMod <module>");
					return;
				}

				CModInfo Info;
				CString sError;
				if (!pZNC->GetModules().GetModInfo(Info, sMod, sError))
					PutError(sError);
				else if (!pZNC->UpdateModule(sMod))
					PutError("module '" + sMod + "' not updated");
				else
					PutSuccess("module '" + sMod + "' updated");
			}
		},
	};

	const std::vector<Command<CUser>> UserCmds = {
		{
			"AddNetwork <name>",
			"Adds a network.",
			[=](CUser* pUser, const CString& sArgs) {
				if (!GetUser()->IsAdmin() && !pUser->HasSpaceForNewNetwork()) {
					PutError("exceeded limit " + CString(pUser->MaxNetworks()));
					return;
				}

				const CString sNetwork = sArgs.Token(0);

				if (sNetwork.empty()) {
					PutUsage("AddNetwork <name>");
					return;
				}
				if (!CIRCNetwork::IsValidNetwork(sNetwork)) {
					PutError("invalid name (must be alphanumeric)");
					return;
				}

				CString sError;
				if (pUser->AddNetwork(sNetwork, sError))
					PutSuccess("network added. Use /znc Jump " + sNetwork + ", or connect to ZNC with username " + pUser->GetUserName() + "/" + sNetwork + " (instead of just " + pUser->GetUserName() + ") to connect to it.");
				else
					PutError(sError);
			}
		},
		{
			"CloneUser <user>",
			"Clones all attributes from the specified user.",
			[=](CUser* pUser, const CString& sArgs) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return;
				}

				if (sArgs.empty()) {
					PutUsage("CloneUser <user>");
					return;
				}

				CUser *pSource = CZNC::Get().FindUser(sArgs);
				if (!pSource) {
					PutError("unknown user");
					return;
				}

				CString sError;
				if (!pUser->Clone(*pSource, sError))
					PutError(sError);
				else
					PutSuccess("cloned");
			}
		},
		{
			"DelNetwork <name>",
			"Deletes a network.",
			[=](CUser* pUser, const CString& sArgs) {
				const CString sNetwork = sArgs.Token(0);

				if (sNetwork.empty()) {
					PutUsage("DelNetwork <name>");
					return;
				}

				if (pUser->DeleteNetwork(sNetwork))
					PutSuccess("network '" + sNetwork + "' deleted");
				else
					PutError("unknown network");
			}
		},
		{
			"ListClients [filter]",
			"Lists connected user clients.",
			[=](CUser* pUser, const CString& sArgs) {
				const CString sFilter = sArgs.Token(1);

				CTable Table;
				Table.AddColumn("Host");
				Table.AddColumn("Name");

				for (const CClient* pClient : pUser->GetAllClients()) {
					if (sFilter.empty()
							|| !pClient->GetRemoteIP().WildCmp(sFilter, CString::CaseInsensitive)
							|| !pClient->GetFullName().WildCmp(sFilter, CString::CaseInsensitive)) {
						Table.AddRow();
						Table.SetCell("Host", pClient->GetRemoteIP());
						Table.SetCell("Name", pClient->GetFullName());
					}
				}

				if (Table.empty()) {
					if (sFilter.empty())
						PutLine("No connected clients");
					else
						PutLine("No matches for '" + sFilter + "'");
				} else {
					PutTable(Table);
				}
			}
		},
		{
			"ListMods [filter]",
			"Lists user modules.",
			[=](CUser* pUser, const CString& sArgs) {
				OnListModsCommand(pUser, sArgs, CModInfo::UserModule);
			}
		},
		{
			"ListNetworks [filter]",
			"Lists user networks.",
			[=](CUser* pUser, const CString& sArgs) {
				const CString sFilter = sArgs.Token(0);

				CTable Table;
				Table.AddColumn("Network");
				Table.AddColumn("Status");

				for (const CIRCNetwork* pNetwork : pUser->GetNetworks()) {
					if (sFilter.empty() || pNetwork->GetName().WildCmp(sFilter, CString::CaseInsensitive)) {
						Table.AddRow();
						Table.SetCell("Network", pNetwork->GetName());
						if (pNetwork->IsIRCConnected())
							Table.SetCell("Status", "Online (" + pNetwork->GetCurrentServer()->GetName() + ")");
						else
							Table.SetCell("Status", pNetwork->GetIRCConnectEnabled() ? "Offline" : "Disabled");
					}
				}

				if (Table.empty()) {
					if (sFilter.empty())
						PutLine("No networks");
					else
						PutLine("No matches for '" + sFilter + "'");
				} else {
					PutTable(Table);
				}
			}
		},
		{
			"LoadMod <module> [args]",
			"Loads a user module.",
			[=](CUser* pUser, const CString& sArgs) {
				OnLoadModCommand(pUser, sArgs, CModInfo::UserModule);
			}
		},
		{
			"ReloadMod <module> [args]",
			"Reloads a user module.",
			[=](CUser* pUser, const CString& sArgs) {
				OnReloadModCommand(pUser, sArgs);
			}
		},
		{
			"Traffic",
			"Shows the amount of user specific traffic.",
			[=](CUser* pUser, const CString& sArgs) {
				CTable Table;
				Table.AddColumn("Network");
				Table.AddColumn("Sent");
				Table.AddColumn("Received");
				Table.AddColumn("Total");
				for (const CIRCNetwork* pNetwork : pUser->GetNetworks()) {
					Table.AddRow();
					Table.SetCell("Network", pNetwork->GetName());
					Table.SetCell("Sent", CString::ToByteStr(pNetwork->BytesWritten()));
					Table.SetCell("Received", CString::ToByteStr(pNetwork->BytesRead()));
					Table.SetCell("Total", CString::ToByteStr(pNetwork->BytesRead() + pNetwork->BytesWritten()));
				}
				PutTable(Table);
			}
		},
		{
			"UnloadMod <module> [args]",
			"Unloads a user module.",
			[=](CUser* pUser, const CString& sArgs) {
				OnUnloadModCommand(pUser, sArgs);
			}
		},
	};

	const std::vector<Command<CIRCNetwork>> NetworkCmds = {
		{
			"AddServer <host> [[+]port] [pass]",
			"Adds an IRC server.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				if (sArgs.empty()) {
					PutUsage("AddServer <host> [[+]port] [pass]");
					return;
				}

				if (pNetwork->AddServer(sArgs))
					PutSuccess("server added");
				else
					PutError("duplicate or invalid entry");
			}
		},
		{
			"CloneNetwork <network> [user]",
			"Clones all attributes from the specified network.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				if (sArgs.empty()) {
					PutUsage("CloneNetwork <network> [user]");
					return;
				}

				const CString sNetwork = sArgs.Token(0);
				const CString sUsername = sArgs.Token(1);

				CUser* pUser = pNetwork->GetUser();
				if (!sUsername.empty())
					pUser = CZNC::Get().FindUser(sUsername);

				if (pUser != GetUser() && !GetUser()->IsAdmin()) {
					PutError("access deniend");
					return;
				}

				if (!pUser) {
					PutError("unknown user");
					return;
				}

				CIRCNetwork* pSource = pUser->FindNetwork(sNetwork);
				if (!pSource) {
					PutError("unknown network");
					return;
				}

				pNetwork->Clone(*pSource, false);
				PutSuccess("cloned");
			}
		},
		{
			"Connect [server]",
			"Connects to an IRC server.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				CServer *pServer = nullptr;
				if (!sArgs.empty()) {
					pServer = pNetwork->FindServer(sArgs);
					if (!pServer) {
						PutError("unknown server");
						return;
					}
					pNetwork->SetNextServer(pServer);

					// if the network is already connecting to
					// a server, the attempt must be aborted
					CIRCSock *pSock = pNetwork->GetIRCSock();
					if (pSock && !pSock->IsConnected())
						pSock->Close();
				}

				CIRCSock* pSock = pNetwork->GetIRCSock();
				if (pSock)
					pSock->Quit();

				if (pServer)
					PutLine("Connecting to '" + pServer->GetName() + "'...");
				else if (pSock)
					PutLine("Jumping to the next server on the list...");
				else
					PutLine("Connecting...");

				pNetwork->SetIRCConnectEnabled(true);
			}
		},
		{
			"DelServer <host> [[+]port] [pass]",
			"Deletes an IRC server.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				if (sArgs.empty()) {
					PutUsage("DelServer <host> [[+]port] [pass]");
					return;
				}

				const CString sHost = sArgs.Token(0);
				const unsigned short uPort = sArgs.Token(1).ToUShort();
				const CString sPass = sArgs.Token(2);

				if (!pNetwork->HasServers())
					PutError("no servers");
				else if (pNetwork->DelServer(sHost, uPort, sPass))
					PutSuccess("server deleted");
				else
					PutError("no such server");
			}
		},
		{
			"Disconnect [message]",
			"Disconnects from the IRC server.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				CIRCSock* pSock = pNetwork->GetIRCSock();
				if (pSock) {
					pSock->Quit(sArgs);
					PutLine("Disconnected");
				} else {
					PutError("not connected");
				}
				pNetwork->SetIRCConnectEnabled(false);
			}
		},
		{
			"ListMods [filter]",
			"Lists network modules.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				OnListModsCommand(pNetwork, sArgs, CModInfo::NetworkModule);
			}
		},
		{
			"ListChans [filter]",
			"Lists all channels of the network.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				const CString sFilter = sArgs.Token(0);

				CTable Table;
				Table.AddColumn("Channel");
				Table.AddColumn("Status");

				for (const CChan* pChan : pNetwork->GetChans()) {
					if (sFilter.empty() || pChan->GetName().WildCmp(sFilter, CString::CaseInsensitive)) {
						Table.AddRow();
						Table.SetCell("Channel", pChan->GetPermStr() + pChan->GetName());
						Table.SetCell("Status", (pChan->IsOn() ? (pChan->IsDetached() ? "Detached" : "Joined") : (pChan->IsDisabled() ? "Disabled" : "Trying")));
					}
				}

				if (Table.empty()) {
					if (sFilter.empty())
						PutLine("No channels");
					else
						PutLine("No matches for '" + sFilter + "'");
				} else {
					PutTable(Table);
				}
			}
		},
		{
			"ListServers [filter]",
			"Lists IRC servers of the network.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				const CString sFilter = sArgs.Token(0);

				CTable Table;
				Table.AddColumn("Server");

				for (const CServer* pServer : pNetwork->GetServers()) {
					if (sFilter.empty() || pServer->GetName().WildCmp(sFilter, CString::CaseInsensitive)) {
						Table.AddRow();
						Table.SetCell("Server", pServer->GetName() + ":" + (pServer->IsSSL() ? "+" : "") + CString(pServer->GetPort()) + (pServer == pNetwork->GetCurrentServer() ? " (current)" : ""));
					}
				}

				if (Table.empty()) {
					if (sFilter.empty())
						PutLine("No servers");
					else
						PutLine("No matches for '" + sFilter + "'");
				} else {
					PutTable(Table);
				}
			}
		},
		{
			"LoadMod <module> [args]",
			"Loads a network module.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				OnLoadModCommand(pNetwork, sArgs, CModInfo::NetworkModule);
			}
		},
		{
			"ReloadMod <module> [args]",
			"Reloads a network module.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				OnReloadModCommand(pNetwork, sArgs);
			}
		},
		{
			"Traffic",
			"Shows the amount of network specific traffic.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				CTable Table;
				Table.AddColumn("Sent");
				Table.AddColumn("Received");
				Table.AddColumn("Total");
				Table.AddRow();
				Table.SetCell("Sent", CString::ToByteStr(pNetwork->BytesWritten()));
				Table.SetCell("Received", CString::ToByteStr(pNetwork->BytesRead()));
				Table.SetCell("Total", CString::ToByteStr(pNetwork->BytesRead() + pNetwork->BytesWritten()));
				PutTable(Table);
			}
		},
		{
			"UnloadMod <module> [args]",
			"Unloads a network module.",
			[=](CIRCNetwork* pNetwork, const CString& sArgs) {
				OnUnloadModCommand(pNetwork, sArgs);
			}
		},
	};

	const std::vector<Command<CChan>> ChanCmds = {
	};
};


CString CAdminMod::GetInfix() const
{
	CString sInfix = GetNV("infix");
	if (sInfix.empty())
		sInfix = GetUser()->GetStatusPrefix();
	return sInfix;
}

void CAdminMod::SetInfix(const CString& sInfix)
{
	SetNV("infix", sInfix);
}

void CAdminMod::OnModCommand(const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	m_sTarget = GetModName();

	if (!GetUser()->IsAdmin() && !sCmd.Equals("Help") && !sCmd.Equals("Get") && !sCmd.Equals("List")) {
		PutError("access denied.");
		return;
	}

	if (sCmd.Equals("Help")) {
		const CString sFilter = sLine.Token(1);

		const CTable Table = FilterCmdTable(GlobalCmds, sFilter);
		if (!Table.empty())
			PutModule(Table);
		else if (!sFilter.empty())
			PutModule("No matches for '" + sFilter + "'");

		const CString sPfx = GetUser()->GetStatusPrefix() + GetInfix();

		if (sFilter.empty()) {
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
		OnListCommand(&CZNC::Get(), sLine, GlobalVars);
	} else if (sCmd.Equals("Get")) {
		OnGetCommand(&CZNC::Get(), sLine, GlobalVars);
	} else if (sCmd.Equals("Set")) {
		OnSetCommand(&CZNC::Get(), sLine, GlobalVars);
	} else if (sCmd.Equals("Reset")) {
		OnResetCommand(&CZNC::Get(), sLine, GlobalVars);
	} else {
		OnExecCommand(&CZNC::Get(), sLine, GlobalCmds);
	}
}

CModule::EModRet CAdminMod::OnUserRaw(CString& sLine)
{
	CString sCopy = sLine;
	if (sCopy.StartsWith("@"))
		sCopy = sCopy.Token(1, true);
	if (sCopy.StartsWith(":"))
		sCopy = sCopy.Token(1, true);

	const CString sCmd = sCopy.Token(0);

	if (sCmd.Equals("ZNC") || sCmd.Equals("PRIVMSG")) {
		CString sTarget = sCopy.Token(1);
		if (sTarget.TrimPrefix(GetUser()->GetStatusPrefix() + GetInfix())) {
			const CString sRest = sCopy.Token(2, true).TrimPrefix_n(":");
			const CString sPfx = GetInfix();

			m_sTarget = GetInfix() + sTarget;

			// <user>
			if (sTarget.Equals("user"))
				return OnUserCommand(GetUser(), sRest);
			if (CUser* pUser = CZNC::Get().FindUser(sTarget))
				return OnUserCommand(pUser, sRest);

			// <network>
			if (sTarget.Equals("network") && GetNetwork())
				return OnNetworkCommand(GetNetwork(), sRest);
			if (CIRCNetwork* pNetwork = GetUser()->FindNetwork(sTarget))
				return OnNetworkCommand(pNetwork, sRest);

			// <#chan>
			if (CChan* pChan = GetNetwork() ? GetNetwork()->FindChan(sTarget) : nullptr)
				return OnChanCommand(pChan, sRest);

			VCString vsParts;
			sTarget.Split("/", vsParts, false);
			if (vsParts.size() == 2) {
				// <user/network>
				if (CUser* pUser = CZNC::Get().FindUser(vsParts[0])) {
					if (CIRCNetwork* pNetwork = pUser->FindNetwork(vsParts[1])) {
						return OnNetworkCommand(pNetwork, sRest);
					} else {
						// <user/#chan>
						if (pUser == GetUser()) {
							if (CIRCNetwork* pUserNetwork = GetNetwork()) {
								if (CChan* pChan = pUserNetwork->FindChan(vsParts[1]))
									return OnChanCommand(pChan, sRest);
							}
						}
						if (pUser->GetNetworks().size() == 1) {
							if (CIRCNetwork* pFirstNetwork = pUser->GetNetworks().front()) {
								if (CChan* pChan = pFirstNetwork->FindChan(vsParts[1]))
									return OnChanCommand(pChan, sRest);
							}
						}
					}
					PutError("unknown (or ambiguous) network or channel");
					return HALT;
				}
				// <network/#chan>
				if (CIRCNetwork* pNetwork = GetUser()->FindNetwork(vsParts[0])) {
					if (CChan* pChan = pNetwork->FindChan(vsParts[1])) {
						return OnChanCommand(pChan, sRest);
					} else {
						PutError("unknown channel");
						return HALT;
					}
				}
			} else if (vsParts.size() == 3) {
				// <user/network/#chan>
				if (CUser* pUser = CZNC::Get().FindUser(vsParts[0])) {
					if (CIRCNetwork* pNetwork = pUser->FindNetwork(vsParts[1])) {
						if (CChan* pChan = pNetwork->FindChan(vsParts[2])) {
							return OnChanCommand(pChan, sRest);
						} else {
							PutError("unknown channel");
							return HALT;
						}
					} else {
						PutError("unknown network");
						return HALT;
					}
				}
			}
		}
	}
	return CONTINUE;
}

CModule::EModRet CAdminMod::OnUserCommand(CUser* pUser, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pUser != GetUser() && !GetUser()->IsAdmin()) {
		PutError("access denied");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sLine, UserCmds);
	else if (sCmd.Equals("List"))
		OnListCommand(pUser, sLine, UserVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(pUser, sLine, UserVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(pUser, sLine, UserVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(pUser, sLine, UserVars);
	else
		OnExecCommand(pUser, sLine, UserCmds);

	return HALT;
}

CModule::EModRet CAdminMod::OnNetworkCommand(CIRCNetwork* pNetwork, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pNetwork->GetUser() != GetUser() && !GetUser()->IsAdmin()) {
		PutError("access denied");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sLine, NetworkCmds);
	else if (sCmd.Equals("List"))
		OnListCommand(pNetwork, sLine, NetworkVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(pNetwork, sLine, NetworkVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(pNetwork, sLine, NetworkVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(pNetwork, sLine, NetworkVars);
	else
		OnExecCommand(pNetwork, sLine, NetworkCmds);

	return HALT;
}

CModule::EModRet CAdminMod::OnChanCommand(CChan* pChan, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pChan->GetNetwork()->GetUser() != GetUser() && !GetUser()->IsAdmin()) {
		PutError("access denied");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sLine, ChanCmds);
	else if (sCmd.Equals("List"))
		OnListCommand(pChan, sLine, ChanVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(pChan, sLine, ChanVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(pChan, sLine, ChanVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(pChan, sLine, ChanVars);
	else
		OnExecCommand(pChan, sLine, ChanCmds);

	return HALT;
}

template <typename C>
void CAdminMod::OnHelpCommand(const CString& sLine, const std::vector<C>& vCmds)
{
	const CString sFilter = sLine.Token(1);

	const CTable Table = FilterCmdTable(vCmds, sFilter);
	if (!Table.empty())
		PutTable(Table);
	else
		PutLine("No matches for '" + sFilter + "'");
}

template <typename T, typename V>
void CAdminMod::OnListCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sFilter = sLine.Token(1);

	const CTable Table = FilterVarTable(vVars, sFilter);
	if (!Table.empty())
		PutTable(Table);
	else
		PutError("unknown variable");
}

template <typename T, typename V>
void CAdminMod::OnGetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sVar = sLine.Token(1);

	if (sVar.empty()) {
		PutUsage("Get <variable>");
		return;
	}

	bool bFound = false;
	for (const auto& Var : vVars) {
		if (Var.name.WildCmp(sVar, CString::CaseInsensitive)) {
			VCString vsValues;
			Var.get(pObject).Split("\n", vsValues, false);
			if (vsValues.empty()) {
				PutLine(Var.name + " = ");
			} else {
				for (const CString& s : vsValues)
					PutLine(Var.name + " = " + s);
			}
			bFound = true;
		}
	}

	if (!bFound)
		PutError("unknown variable");
}

template <typename T, typename V>
void CAdminMod::OnSetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sVar = sLine.Token(1);
	const CString sVal = sLine.Token(2, true);

	if (sVar.empty() || sVal.empty()) {
		PutUsage("Set <variable> <value>");
		return;
	}

	bool bFound = false;
	for (const auto& Var : vVars) {
		if (Var.name.WildCmp(sVar, CString::CaseInsensitive)) {
			if (Var.set(pObject, sVal)) {
				VCString vsValues;
				Var.get(pObject).Split("\n", vsValues, false);
				if (vsValues.empty()) {
					PutLine(Var.name + " = ");
				} else {
					for (const CString& s : vsValues)
						PutLine(Var.name + " = " + s);
				}
			}
			bFound = true;
		}
	}

	if (!bFound)
		PutError("unknown variable");
}

template <typename T, typename V>
void CAdminMod::OnResetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sVar = sLine.Token(1);

	if (sVar.empty()) {
		PutUsage("Reset <variable>");
		return;
	}

	bool bFound = false;
	for (const auto& Var : vVars) {
		if (Var.name.WildCmp(sVar, CString::CaseInsensitive)) {
			if (!Var.reset) {
				PutError("reset not supported");
			} else if (Var.reset(pObject)) {
				VCString vsValues;
				Var.get(pObject).Split("\n", vsValues, false);
				if (vsValues.empty()) {
					PutLine(Var.name + " = ");
				} else {
					for (const CString& s : vsValues)
						PutLine(Var.name + " = " + s);
				}
			}
			bFound = true;
		}
	}

	if (!bFound)
		PutError("unknown variable");
}

template <typename T, typename C>
void CAdminMod::OnExecCommand(T* pObject, const CString& sLine, const std::vector<C>& vCmds)
{
	const CString sCmd = sLine.Token(0);
	const CString sArgs = sLine.Token(1, true);

	for (const auto& Cmd : vCmds) {
		if (Cmd.syntax.Token(0).Equals(sCmd)) {
			Cmd.exec(pObject, sArgs);
			return;
		}
	}

	PutError("unknown command");
}

template <typename T>
void CAdminMod::OnListModsCommand(T* pObject, const CString& sArgs, CModInfo::EModuleType eType)
{
	const CString sFilter = sArgs.Token(0);

	std::set<CModInfo> sMods;
	pObject->GetModules().GetAvailableMods(sMods, eType);

	CTable Table;
	Table.AddColumn("Module");
	Table.AddColumn("Description");

	for (const CModInfo& Info : sMods) {
		const CString& sName = Info.GetName();
		if (sFilter.empty() || sName.StartsWith(sFilter) || sName.WildCmp(sFilter, CString::CaseInsensitive)) {
			Table.AddRow();
			if (pObject->GetModules().FindModule(sName))
				Table.SetCell("Module", sName + " (loaded)");
			else
				Table.SetCell("Module", sName);
			Table.SetCell("Description", Info.GetDescription().Ellipsize(128));
		}
	}

	if (Table.empty())
		PutError("no matches for '" + sFilter + "'");
	else
		PutTable(Table);
}

template <typename T>
void CAdminMod::OnLoadModCommand(T* pObject, const CString& sArgs, CModInfo::EModuleType eType)
{
	if (!GetUser()->IsAdmin() && GetUser()->DenyLoadMod()) {
		PutError("access deniend");
		return;
	}

	const CString sMod = sArgs.Token(0);
	if (sMod.empty()) {
		PutUsage("LoadMod <module> [args]");
		return;
	}

	CModInfo Info;
	CString sError;
	if (!pObject->GetModules().GetModInfo(Info, sMod, sError))
		PutError(sError);
	else if (!pObject->GetModules().LoadModule(sMod, sArgs.Token(1, true), eType, nullptr, nullptr, sError))
		PutError(sError);
	else
		PutSuccess("module '" + sMod + "' loaded");
}

template <typename T>
void CAdminMod::OnReloadModCommand(T* pObject, const CString& sArgs)
{
	if (!GetUser()->IsAdmin() && GetUser()->DenyLoadMod()) {
		PutError("access deniend");
		return;
	}

	const CString sMod = sArgs.Token(0);
	if (sMod.empty()) {
		PutUsage("ReloadMod <module> [args]");
		return;
	}

	CModInfo Info;
	CString sError;
	if (!pObject->GetModules().GetModInfo(Info, sMod, sError))
		PutError(sError);
	else if (!pObject->GetModules().ReloadModule(sMod, sArgs.Token(1, true), nullptr, nullptr, sError))
		PutError(sError);
	else
		PutSuccess("module '" + sMod + "' reloaded");
}

template <typename T>
void CAdminMod::OnUnloadModCommand(T* pObject, const CString& sArgs)
{
	if (!GetUser()->IsAdmin() && GetUser()->DenyLoadMod()) {
		PutError("access deniend");
		return;
	}

	const CString sMod = sArgs.Token(0);
	if (sMod.empty()) {
		PutUsage("UnloadMod <module> [args]");
		return;
	}

	CModInfo Info;
	CString sError;
	if (!pObject->GetModules().GetModInfo(Info, sMod, sError))
		PutError(sError);
	else if (!pObject->GetModules().UnloadModule(sMod, sError))
		PutError(sError);
	else
		PutSuccess("module '" + sMod + "' unloaded");
}

template <typename C>
CTable CAdminMod::FilterCmdTable(const std::vector<C>& vCmds, const CString& sFilter) const
{
	std::map<CString, CString> mCommands;

	if (sFilter.empty() || CString("Get").WildCmp(sFilter, CString::CaseInsensitive))
		mCommands["Get <variable>"] = "Gets the value of a variable.";
	if (sFilter.empty() || CString("Help").WildCmp(sFilter, CString::CaseInsensitive))
		mCommands["Help [filter]"] = "Generates this output.";
	if (sFilter.empty() || CString("List").WildCmp(sFilter, CString::CaseInsensitive))
		mCommands["List [filter]"] = "Lists available variables filtered by name or type.";
	if (sFilter.empty() || CString("Reset").WildCmp(sFilter, CString::CaseInsensitive))
		mCommands["Reset <variable>"] = "Resets the value of a variable.";
	if (sFilter.empty() || CString("Set").WildCmp(sFilter, CString::CaseInsensitive))
		mCommands["Set <variable> <value>"] = "Sets the value of a variable.";

	for (const auto& Cmd : vCmds) {
		const CString sCmd =  Cmd.syntax.Token(0);
		if (sFilter.empty() || sCmd.StartsWith(sFilter) || sCmd.WildCmp(sFilter, CString::CaseInsensitive))
			mCommands[Cmd.syntax] = Cmd.description;
	}

	CTable Table;
	Table.AddColumn("Command");
	Table.AddColumn("Description");

	for (const auto& it : mCommands) {
		Table.AddRow();
		Table.SetCell("Command", it.first);
		Table.SetCell("Description", it.second);
	}

	return Table;
}

template <typename V>
CTable CAdminMod::FilterVarTable(const std::vector<V>& vVars, const CString& sFilter) const
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

void CAdminMod::PutSuccess(const CString& sLine, const CString& sTarget)
{
	PutLine("Success: " + sLine, sTarget);
}

void CAdminMod::PutUsage(const CString& sSyntax, const CString& sTarget)
{
	PutLine("Usage: " + sSyntax, sTarget);
}

void CAdminMod::PutError(const CString& sError, const CString& sTarget)
{
	PutLine("Error: " + sError, sTarget);
}

void CAdminMod::PutLine(const CString& sLine, const CString& sTarget)
{
	const CString sTgt = sTarget.empty() ? (m_sTarget.empty() ? GetModName() : m_sTarget) : sTarget;

	if (CClient* pClient = GetClient())
		pClient->PutModule(sTgt, sLine);
	else if (CIRCNetwork* pNetwork = GetNetwork())
		pNetwork->PutModule(sTgt, sLine);
	else if (CUser* pUser = GetUser())
		pUser->PutModule(sTgt, sLine);
}

void CAdminMod::PutTable(const CTable& Table, const CString& sTarget)
{
	CString sLine;
	unsigned int i = 0;
	while (Table.GetLine(i++, sLine))
		PutLine(sLine, sTarget);
}

template<> void TModInfo<CAdminMod>(CModInfo& Info) {
}

USERMODULEDEFS(CAdminMod, "Administer ZNC conveniently through IRC.")
