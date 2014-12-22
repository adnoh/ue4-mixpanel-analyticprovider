// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MixpanelPrivatePCH.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnalytics, Display, All);

IMPLEMENT_MODULE(FAnalyticsMixpanel, Mixpanel)

TSharedPtr< FAnalyticsProviderMixpanel > FAnalyticsMixpanel::Provider;

void FAnalyticsMixpanel::StartupModule()
{
}

void FAnalyticsMixpanel::ShutdownModule()
{
	Provider.Reset();
	Provider = NULL;
}

TSharedPtr<IAnalyticsProvider> FAnalyticsMixpanel::CreateAnalyticsProvider(const FAnalytics::FProviderConfigurationDelegate& GetConfigValue) const
{
	if ( !Provider.IsValid() )
	{
		if (GetConfigValue.IsBound())
		{
			const FString Token = GetConfigValue.Execute(TEXT("ApiToken"), true);
			Provider = MakeShareable(new FAnalyticsProviderMixpanel(Token));
		}
		else
		{
			UE_LOG(LogAnalytics, Warning, TEXT("FAnalyticsMixpanel::CreateAnalyticsProvider called with an unbound delegate"));
		}
	}
	
	return Provider;
}

FAnalyticsProviderMixpanel::FAnalyticsProviderMixpanel(const FString Token) :
	ApiToken(Token),
	bHasSessionStarted(false)
{
	UserId = FPlatformMisc::GetUniqueDeviceId();

	UE_LOG(LogAnalytics, Display, TEXT("FAnalyticsMixpanel::ctor user (%s) and ApiToken (%s)"), *UserId, *ApiToken);
}

FAnalyticsProviderMixpanel::~FAnalyticsProviderMixpanel()
{
	if (bHasSessionStarted)
	{
		EndSession();
	}
}

bool FAnalyticsProviderMixpanel::StartSession(const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (bHasSessionStarted)
	{
		EndSession();
	}
	if (ApiToken != "" && UserId != "")
	{
		SessionId = UserId + TEXT("-") + FDateTime::Now().ToString();
		bHasSessionStarted = true;
		UE_LOG(LogAnalytics, Display, TEXT("Session started for user (%s) and session id (%s)"), *UserId, *SessionId);
	}
	return bHasSessionStarted;
}

void FAnalyticsProviderMixpanel::EndSession()
{
	UE_LOG(LogAnalytics, Display, TEXT("Session ended for user (%s) and session id (%s)"), *UserId, *SessionId);
	bHasSessionStarted = false;
}

void FAnalyticsProviderMixpanel::FlushEvents()
{
	UE_LOG(LogAnalytics, Display, TEXT("Analytics flushed"));
}

void FAnalyticsProviderMixpanel::SetUserID(const FString& InUserID)
{
	if (!bHasSessionStarted)
	{
		UserId = InUserID;
		UE_LOG(LogAnalytics, Display, TEXT("User is now (%s)"), *UserId);
	}
	else
	{
		// Log that we shouldn't switch users during a session
		UE_LOG(LogAnalytics, Warning, TEXT("FAnalyticsProviderMixpanel::SetUserID called while a session is in progress. Ignoring."));
	}
}

FString FAnalyticsProviderMixpanel::GetUserID() const
{
	return UserId;
}

FString FAnalyticsProviderMixpanel::GetSessionID() const
{
	return SessionId;
}

bool FAnalyticsProviderMixpanel::SetSessionID(const FString& InSessionID)
{
	if (!bHasSessionStarted)
	{
		SessionId = InSessionID;
		UE_LOG(LogAnalytics, Display, TEXT("Session is now (%s)"), *SessionId);
	}
	else
	{
		// Log that we shouldn't switch session ids during a session
		UE_LOG(LogAnalytics, Warning, TEXT("FAnalyticsProviderMixpanel::SetSessionID called while a session is in progress. Ignoring."));
	}
	return !bHasSessionStarted;
}

void FAnalyticsProviderMixpanel::RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (bHasSessionStarted)
	{
		// Create the HTTP request
		TSharedRef< IHttpRequest > HttpRequest = FHttpModule::Get().CreateRequest();
		HttpRequest->SetVerb("GET");

		FString outStr;
		TSharedRef<TJsonWriter<TCHAR>> JsonWriter = TJsonWriterFactory<TCHAR>::Create(&outStr);
		JsonWriter->WriteObjectStart();
		{
			JsonWriter->WriteValue(FAnalyticsMixpanel::GetKeyNameForEventName(), EventName);
			JsonWriter->WriteObjectStart(FAnalyticsMixpanel::GetKeyNameForProperties());
			{
				JsonWriter->WriteValue(FAnalyticsMixpanel::GetKeyNameForDistinctId(), UserId);
				JsonWriter->WriteValue(FAnalyticsMixpanel::GetKeyNameForToken(), ApiToken);

				JsonWriter->WriteValue(FAnalyticsMixpanel::GetKeyNameForSessionIdProperty(), SessionId);
				if (Attributes.Num() > 0)
				{
					// Write out the list of attributes as an array of attribute objects
					for (auto Attr : Attributes)
					{
						JsonWriter->WriteValue(*Attr.AttrName, *Attr.AttrValue);
					}
				}
			}
			JsonWriter->WriteObjectEnd();
		}
		JsonWriter->WriteObjectEnd();
		JsonWriter->Close();

		UE_LOG(LogAnalytics, Display, TEXT("FAnalyticsProviderMixpanel::RecordEvent Post data: %s"), *outStr);
		
		HttpRequest->SetURL(FAnalyticsMixpanel::GetAPITrackURL() + FBase64::Encode(outStr));
		HttpRequest->OnProcessRequestComplete().BindRaw(this, &FAnalyticsProviderMixpanel::EventRequestComplete);
		// Execute the request
		HttpRequest->ProcessRequest();
	}
	else
	{
		// Log that we shouldn't switch session ids during a session
		UE_LOG(LogAnalytics, Warning, TEXT("FAnalyticsProviderMixpanel::RecordEvent called while a session is not started. Ignoring."));
	}
}

void FAnalyticsProviderMixpanel::RecordItemPurchase(const FString& ItemId, const FString& Currency, int PerItemCost, int ItemQuantity)
{
	TArray<FAnalyticsEventAttribute> attributes;
	attributes.Add(FAnalyticsEventAttribute("ItemId", ItemId));
	attributes.Add(FAnalyticsEventAttribute("Currency", Currency));
	attributes.Add(FAnalyticsEventAttribute("PerItemCost", PerItemCost));
	attributes.Add(FAnalyticsEventAttribute("ItemQuantity", ItemQuantity));

	RecordEvent("Monetization ItemPurchase", attributes);
}

void FAnalyticsProviderMixpanel::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const FString& RealCurrencyType, float RealMoneyCost, const FString& PaymentProvider)
{
	TArray<FAnalyticsEventAttribute> attributes;
	attributes.Add(FAnalyticsEventAttribute("GameCurrencyType", GameCurrencyType));
	attributes.Add(FAnalyticsEventAttribute("GameCurrencyAmount", GameCurrencyAmount));
	attributes.Add(FAnalyticsEventAttribute("RealCurrencyType", RealCurrencyType));
	attributes.Add(FAnalyticsEventAttribute("RealMoneyCost", RealMoneyCost));
	attributes.Add(FAnalyticsEventAttribute("PaymentProvider", PaymentProvider));

	RecordEvent("Monetization CurrencyPurchase", attributes);
}

void FAnalyticsProviderMixpanel::RecordCurrencyGiven(const FString& GameCurrencyType, int GameCurrencyAmount)
{
	TArray<FAnalyticsEventAttribute> attributes;
	attributes.Add(FAnalyticsEventAttribute("GameCurrencyType", GameCurrencyType));
	attributes.Add(FAnalyticsEventAttribute("GameCurrencyAmount", GameCurrencyAmount));

	RecordEvent("Monetization CurrencyGiven", attributes);
}

void FAnalyticsProviderMixpanel::EventRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (bSucceeded && HttpResponse.IsValid())
	{
		UE_LOG(LogAnalytics, Display, TEXT("Calc response for [%s]. Code: %d. Payload: %s"), *HttpRequest->GetURL(), HttpResponse->GetResponseCode(), *HttpResponse->GetContentAsString());
	}
	else
	{
		UE_LOG(LogAnalytics, Display, TEXT("Calc response for [%s]. No response"), *HttpRequest->GetURL());
	}
}