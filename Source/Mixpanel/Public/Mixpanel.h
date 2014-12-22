// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProviderModule.h"
#include "Core.h"

class IAnalyticsProvider;
class FAnalyticsProviderMixpanel;

/**
* The public interface to this module
*/
class FAnalyticsMixpanel : public IAnalyticsProviderModule
{
	//--------------------------------------------------------------------------
	// Mixpanel constants
	//--------------------------------------------------------------------------
public:
	static FString GetAPITrackURL() { return TEXT("http://api.mixpanel.com/track/?data="); }
	static FString GetKeyNameForToken() { return TEXT("token"); }
	static FString GetKeyNameForDistinctId() { return TEXT("distinct_id"); }
	static FString GetKeyNameForEventName() { return TEXT("event"); }
	static FString GetKeyNameForProperties() { return TEXT("properties"); }
	static FString GetKeyNameForSessionIdProperty() { return TEXT("session_id"); }
	
	//--------------------------------------------------------------------------
	// Module functionality
	//--------------------------------------------------------------------------
public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FAnalyticsMixpanel& Get()
	{
		return FModuleManager::LoadModuleChecked< FAnalyticsMixpanel >("Mixpanel");
	}
	
	//--------------------------------------------------------------------------
	// provider factory functions
	//--------------------------------------------------------------------------
public:
	/**
	* IAnalyticsProviderModule interface.
	* Creates the analytics provider given a configuration delegate.
	* The keys required exactly match the field names in the Config object.
	*/
	virtual TSharedPtr<IAnalyticsProvider> CreateAnalyticsProvider(const FAnalytics::FProviderConfigurationDelegate& GetConfigValue) const override;

private:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static TSharedPtr< FAnalyticsProviderMixpanel > Provider;
};