/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WINDOWSSANDBOX_H
#define __WINDOWSSANDBOX_H

#include <windows.h>
#include "Dacl.h"
#include "Sid.h"
#include "UniqueHandle.h"
#include <vector>

namespace mozilla {

typedef UNIQUE_HANDLE_TYPE(HANDLE, &::CloseHandle) ScopedHandle;

class WindowsSandbox
{
public:
  WindowsSandbox() {}
  virtual ~WindowsSandbox() {}

  bool Init(int aArgc, wchar_t* aArgv[]);
  void Fini();

  static const wchar_t DESKTOP_NAME[];
  static const wchar_t SWITCH_IMPERSONATION_TOKEN_HANDLE[];
  static const wchar_t SWITCH_JOB_HANDLE[];

protected:
  virtual DWORD64 GetDeferredMitigationPolicies() { return 0; }
  virtual bool OnPrivInit() = 0;
  virtual bool OnInit() = 0;
  virtual void OnFini() = 0;

private:
  bool ValidateJobHandle(HANDLE aJob);
  bool SetMitigations(const DWORD64 aMitigations);
  bool DropProcessIntegrityLevel();
};

class WindowsSandboxLauncher
{
public:
  WindowsSandboxLauncher();
  virtual ~WindowsSandboxLauncher();

  enum InitFlags
  {
    eInitNormal = 0,
    eInitNoSeparateWindowStation = 1
  };

  bool Init(InitFlags aInitFlags = eInitNormal,
            DWORD64 aMitigationPolicies = DEFAULT_MITIGATION_POLICIES);

  inline void AddHandleToInherit(HANDLE aHandle)
  {
    if (aHandle) {
      mHandlesToInherit.push_back(aHandle);
    }
  }
  bool Launch(const wchar_t* aExecutablePath, const wchar_t* aBaseCmdLine);
  bool Wait(unsigned int aTimeoutMs) const;
  bool IsSandboxRunning() const;
  bool GetInheritableSecurityDescriptor(SECURITY_ATTRIBUTES& aSa,
                                        const BOOL aInheritable = TRUE);

  static const DWORD64 DEFAULT_MITIGATION_POLICIES;

protected:
  virtual bool PreResume() { return true; }

private:
  bool CreateSidList(HANDLE aToken, SID_AND_ATTRIBUTES*& aOutput,
                     unsigned int& aNumSidAttrs, unsigned int aFilterFlags,
                     mozilla::Sid* aLogonSid = nullptr);
  void FreeSidList(SID_AND_ATTRIBUTES* aListToFree);
  bool CreateTokens(const Sid& aCustomSid, ScopedHandle& aRestrictedToken,
                    ScopedHandle& aImpersonationToken, Sid& aLogonSid);
  HWINSTA CreateWindowStation();
  std::unique_ptr<wchar_t[]> GetWindowStationName(HWINSTA aWinsta);
  HDESK CreateDesktop(HWINSTA aWinsta, const Sid& aCustomSid);
  bool CreateJob(ScopedHandle& aJob);
  bool GetWorkingDirectory(ScopedHandle& aToken, wchar_t* aBuf, size_t aBufLen);
  std::unique_ptr<wchar_t[]> CreateAbsolutePath(const wchar_t* aInputPath);
  bool BuildInheritableSecurityDescriptor(const Sid& aLogonSid);

  InitFlags mInitFlags;
  std::vector<HANDLE> mHandlesToInherit;
  bool    mHasWinVistaAPIs;
  bool    mHasWin8APIs;
  bool    mHasWin10APIs;
  DWORD64 mMitigationPolicies;
  HANDLE  mProcess;
  HWINSTA mWinsta;
  HDESK   mDesktop;
  Dacl    mInheritableDacl;
  SECURITY_DESCRIPTOR mInheritableSd;
};

} // namespace mozilla

#endif // __WINDOWSSANDBOX_H

