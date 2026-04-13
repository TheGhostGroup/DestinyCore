/*
 * This file is part of the DestinyCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AuthenticationPackets.h"
#include "BattlenetRpcErrorCodes.h"
#include "ObjectMgr.h"
#include "CharacterData.h"
#include "ClientConfigPackets.h"
#include "SystemPackets.h"
#include "CharacterPackets.h"
#include "TokenPackets.h"
#include "BattlePayMgr.h"

void WorldSession::SendAuthWaitQue(uint32 position)
{
    if (position)
    {
        WorldPackets::Auth::WaitQueueUpdate waitQueueUpdate;
        waitQueueUpdate.WaitInfo.WaitCount = position;
        waitQueueUpdate.WaitInfo.WaitTime = 0;
        waitQueueUpdate.WaitInfo.HasFCM = false;
        SendPacket(waitQueueUpdate.Write());
    }
    else
        SendPacket(WorldPackets::Auth::WaitQueueFinish().Write());
}

void WorldSession::SendAuthResponse(uint8 code, bool CharacterTemplate, bool queued /*= false*/, uint32 queuePos /*= 0*/)
{
    WorldPackets::Auth::AuthResponse response;
    response.Result = code;

    if (code == ERROR_OK)
    {
        response.SuccessInfo.emplace();

        response.SuccessInfo->AccountExpansionLevel = Expansion();
        response.SuccessInfo->ActiveExpansionLevel = Expansion();
        response.SuccessInfo->VirtualRealmAddress = GetVirtualRealmAddress();
        response.SuccessInfo->CurrencyID = GetBattlePayMgr()->GetShopCurrency();
        response.SuccessInfo->VirtualRealms.emplace_back(GetVirtualRealmAddress(), true, false, sObjectMgr->GetRealmName(realm.Id.Realm), sObjectMgr->GetNormalizedRealmName(realm.Id.Realm));
        response.SuccessInfo->AvailableClasses = &sObjectMgr->GetClassExpansionRequirements();
        response.SuccessInfo->Time = int32(GameTime::GetGameTime());

		
        if (CharacterTemplate && sWorld->getBoolConfig(CONFIG_CHARACTER_TEMPLATE))
        {
            if (realm.Build < 26972)
            {
				for (auto const& templ : sCharacterDataStore->GetCharacterTemplates())
				{
                WorldPackets::Auth::AuthResponse::CharacterTemplateData templateData;
                templateData.TemplateSetID = templ.second.TemplateSetID;
                for (auto x : templ.second.Classes)
                    templateData.Classes.emplace_back(x.FactionGroup, x.ClassID);
                for (auto z : templ.second.Items)
                    templateData.Items.emplace_back(z.ItemID, z.Count, z.ClassID, z.FactionGroup);
                templateData.Name = templ.second.Name;
                templateData.Description = templ.second.Description;
                response.SuccessInfo->Templates.emplace_back(templateData);
				}
            }
        }
        else
        {
            for (auto& templ : charTemplateData)
            {
                if (!templ.second.active)
                    continue;

                auto charTemplate = sCharacterDataStore->GetCharacterTemplate(templ.second.templateId);
                if (!charTemplate)
                    continue;

                WorldPackets::Auth::AuthResponse::CharacterTemplateData templateData;
                templateData.TemplateSetID = templ.second.id;
                for (auto x : charTemplate->Classes)
                    templateData.Classes.emplace_back(x.FactionGroup, x.ClassID);
                for (auto z : charTemplate->Items)
                    templateData.Items.emplace_back(z.ItemID, z.Count, z.ClassID, z.FactionGroup);
                templateData.Name = { std::to_string(templ.second.level) + " level " + std::to_string(templ.second.iLevel) + " ilevel" };
                templateData.Description = { "Create character with " + std::to_string(templ.second.level) + " level and " + std::to_string(templ.second.iLevel) + "ilvl items" };
                response.SuccessInfo->Templates.emplace_back(templateData);
            }
        }
    }

    if (queued)
    {
        response.WaitInfo.emplace();
        response.WaitInfo->WaitCount = queuePos;
    }

    SendPacket(response.Write());
}

void WorldSession::SendClientCacheVersion(uint32 version)
{
    WorldPackets::ClientConfig::ClientCacheVersion cache;
    cache.CacheVersion = version;
    SendPacket(cache.Write());
}

void WorldSession::SendFeatureSystemStatusGlueScreen()
{
    WorldPackets::System::FeatureSystemStatusGlueScreen features;
    features.TokenPollTimeSeconds = 300;
    features.TokenRedeemIndex = 0; // ForSubAmount30Days
    features.BpayStoreAvailable = sWorld->getBoolConfig(CONFIG_FEATURE_SYSTEM_BPAY_STORE_ENABLED);
    features.BpayStoreDisabledByParentalControls = false;
    features.CharUndeleteEnabled = HasAuthFlag(AT_AUTH_FLAG_RESTORE_DELETED_CHARACTER);
    features.BpayStoreEnabled = sWorld->getBoolConfig(CONFIG_FEATURE_SYSTEM_BPAY_STORE_ENABLED);
    features.CommerceSystemEnabled = true;
    features.Unk14 = true;
    features.WillKickFromWorld = false;
    features.KioskModeEnabled = false;
    features.TrialBoostEnabled = sWorld->getBoolConfig(CONFIG_CLASS_TRIAL_ENABLED);
    features.IsExpansionPreorderInStore = false;
    features.CompetitiveModeEnabled = false;
    features.TokenBalanceEnabled = true;
    features.LiveRegionCharacterListEnabled = false;
    features.LiveRegionCharacterCopyEnabled = false;
    features.LiveRegionAccountCopyEnabled = false;
    features.TokenBalanceAmount = 5500000;
    features.BpayStoreProductDeliveryDelay = 180;
    features.UnkInt1 = 3;
    features.UnkInt2 = 2;
    features.UnkInt3 = 5;
    features.UnkInt4 = 7;
    SendPacket(features.Write());
}

void WorldSession::HandleWowTokenMarketPrice(WorldPackets::Token::RequestWowTokenMarketPrice& packet)
{
    WorldPackets::Token::WowTokenMarketPriceResponse response;
    response.CurrentMarketPrice = static_cast<uint64>(sWorld->getIntConfig(CONFIG_WOW_TOKEN_MARKET_PRICE)) * GOLD;
    response.Result = sWorld->getBoolConfig(CONFIG_WOW_TOKEN_ENABLED) ? TOKEN_RESULT_SUCCESS : TOKEN_RESULT_ERROR_DISABLED;
    response.UnkInt = packet.UnkInt;
    response.UnkInt2 = 14400;
    SendPacket(response.Write());
}

void WorldSession::HandleUpdateListedAuctionableTokens(WorldPackets::Token::UpdateListedAuctionableTokens& packet)
{
    WorldPackets::Token::UpdateListedAuctionableTokensResponse response;
    response.UnkInt = packet.Type;

    if (!sWorld->getBoolConfig(CONFIG_WOW_TOKEN_ENABLED))
    {
        response.Result = TOKEN_RESULT_ERROR_DISABLED;
        SendPacket(response.Write());
        return;
    }

    response.Result = TOKEN_RESULT_SUCCESS;

    WorldPackets::Token::UpdateListedAuctionableTokensResponse::AuctionableTokenAuctionable token;
    token.BuyoutPrice = static_cast<uint64>(sWorld->getIntConfig(CONFIG_WOW_TOKEN_MARKET_PRICE)) * GOLD;
    token.DistributionID = 1;
    token.DateCreated = static_cast<uint32>(GameTime::GetGameTime());
    token.Owner = 0;
    token.EndTime = static_cast<uint32>(GameTime::GetGameTime()) + 14400;
    response.AuctionableTokenAuctionableList.push_back(token);

    SendPacket(response.Write());
}

void WorldSession::HandleCheckVeteranTokenEligibility(WorldPackets::Token::CheckVeteranTokenEligibility& packet)
{
    WorldPackets::Token::WowTokenCanVeteranBuyResult result;
    result.UnkLong = 0;
    result.UnkInt = packet.UnkInt;
    result.UnkInt2 = 1;
    SendPacket(result.Write());
}

void WorldSession::HandleBuyWowTokenStart(WorldPackets::Token::BuyWowTokenStart& packet)
{
    WorldPackets::Token::WowTokenBuyRequestConfirmation response;

    if (!sWorld->getBoolConfig(CONFIG_WOW_TOKEN_ENABLED))
    {
        response.Result = TOKEN_RESULT_ERROR_DISABLED;
        SendPacket(response.Write());
        return;
    }

    Player* player = GetPlayer();
    if (!player)
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    uint64 price = static_cast<uint64>(sWorld->getIntConfig(CONFIG_WOW_TOKEN_MARKET_PRICE)) * GOLD;

    if (static_cast<uint64>(player->GetMoney()) < static_cast<uint64>(sWorld->getIntConfig(CONFIG_WOW_TOKEN_MARKET_PRICE)) * GOLD)
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    response.CurrentMarketPrice = price;
    response.TokenCount = 1;
    response.Result = TOKEN_RESULT_SUCCESS;
    SendPacket(response.Write());
}

void WorldSession::HandleBuyWowTokenConfirm(WorldPackets::Token::BuyWowTokenConfirm& packet)
{
    WorldPackets::Token::WowTokenBuyResultConfirmation response;
    response.UnkInt = packet.UnkInt;

    if (!sWorld->getBoolConfig(CONFIG_WOW_TOKEN_ENABLED) || !packet.Confirmed)
    {
        response.Result = TOKEN_RESULT_ERROR_DISABLED;
        SendPacket(response.Write());
        return;
    }

    Player* player = GetPlayer();

    if (!player)
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    uint64 price = static_cast<uint64>(sWorld->getIntConfig(CONFIG_WOW_TOKEN_MARKET_PRICE)) * GOLD;

    if (static_cast<uint64>(player->GetMoney()) < price)
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    uint32 tokenItemId = sWorld->getIntConfig(CONFIG_WOW_TOKEN_ITEM_ID);
    if (!player->AddItem(tokenItemId, 1))
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    player->ModifyMoney(-static_cast<int64>(price));
    response.Result = TOKEN_RESULT_SUCCESS;
    SendPacket(response.Write());
}

void WorldSession::HandleSellWowTokenStart(WorldPackets::Token::SellWowTokenStart& packet)
{
    WorldPackets::Token::WowTokenSellRequestConfirmation response;

    if (!sWorld->getBoolConfig(CONFIG_WOW_TOKEN_ENABLED))
    {
        response.Result = TOKEN_RESULT_ERROR_DISABLED;
        SendPacket(response.Write());
        return;
    }

    Player* player = GetPlayer();
    if (!player)
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    uint32 tokenItemId = sWorld->getIntConfig(CONFIG_WOW_TOKEN_ITEM_ID);
    if (!player->HasItemCount(tokenItemId, 1))
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    response.GuaranteedPrice = static_cast<uint64>(sWorld->getIntConfig(CONFIG_WOW_TOKEN_MARKET_PRICE)) * GOLD;
    response.UnkInt = packet.UnkInt;
    response.Result = TOKEN_RESULT_SUCCESS;
    SendPacket(response.Write());
}

void WorldSession::HandleSellWowTokenConfirm(WorldPackets::Token::SellWowTokenConfirm& packet)
{
    WorldPackets::Token::WowTokenSellResultConfirmation response;
    response.UnkInt = packet.UnkInt;

    if (!sWorld->getBoolConfig(CONFIG_WOW_TOKEN_ENABLED) || !packet.Confirmed)
    {
        response.Result = TOKEN_RESULT_ERROR_DISABLED;
        SendPacket(response.Write());
        return;
    }

    Player* player = GetPlayer();
    if (!player)
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    uint32 tokenItemId = sWorld->getIntConfig(CONFIG_WOW_TOKEN_ITEM_ID);
    if (!player->HasItemCount(tokenItemId, 1))
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    player->DestroyItemCount(tokenItemId, 1, true);

    uint64 price = packet.GuaranteedPrice;
    if (price == 0)
        price = static_cast<uint64>(sWorld->getIntConfig(CONFIG_WOW_TOKEN_MARKET_PRICE)) * GOLD;

    player->ModifyMoney(static_cast<int64>(price));
    response.Result = TOKEN_RESULT_SUCCESS;
    SendPacket(response.Write());
}

void WorldSession::HandleRedeemWowTokenStart(WorldPackets::Token::RedeemWowTokenStart& packet)
{
    WorldPackets::Token::WowTokenRedeemRequestConfirmation response;

    if (!sWorld->getBoolConfig(CONFIG_WOW_TOKEN_ENABLED))
    {
        response.Result = TOKEN_RESULT_ERROR_DISABLED;
        SendPacket(response.Write());
        return;
    }

    Player* player = GetPlayer();
    if (!player)
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    uint32 tokenItemId = sWorld->getIntConfig(CONFIG_WOW_TOKEN_ITEM_ID);
    if (!player->HasItemCount(tokenItemId, 1))
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    response.Count = packet.Count;
    response.UnkInt = packet.UnkInt;
    response.Result = TOKEN_RESULT_SUCCESS;
    SendPacket(response.Write());
}

void WorldSession::HandleRedeemWowTokenConfirm(WorldPackets::Token::RedeemWowTokenConfirm& packet)
{
    WorldPackets::Token::WowTokenRedeemResult response;
    response.UnkInt = packet.UnkInt;

    if (!sWorld->getBoolConfig(CONFIG_WOW_TOKEN_ENABLED) || !packet.Confirm)
    {
        response.Result = TOKEN_RESULT_ERROR_DISABLED;
        SendPacket(response.Write());
        return;
    }

    Player* player = GetPlayer();
    if (!player)
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    uint32 tokenItemId = sWorld->getIntConfig(CONFIG_WOW_TOKEN_ITEM_ID);
    if (!player->HasItemCount(tokenItemId, 1))
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    player->DestroyItemCount(tokenItemId, 1, true);

    uint32 redeemAmount = static_cast<uint32>(sWorld->getIntConfig(CONFIG_WOW_TOKEN_REDEEM_BALANCE));
    if (!player->AddDonateTokenCount(redeemAmount))
    {
        response.Result = TOKEN_RESULT_ERROR_OTHER;
        SendPacket(response.Write());
        return;
    }

    response.Result = TOKEN_RESULT_SUCCESS;
    SendPacket(response.Write());

    SendFeatureSystemStatusGlueScreen();
}

void WorldSession::HandleUpdateWowTokenCount(WorldPackets::Token::UpdateWowTokenCount& packet)
{
    WorldPackets::Token::WowTokenDistributionGlueUpdate response;
    response.UnkInt = packet.UnkInt;
    response.UnkInt2 = 0;
    response.DistributionCount1 = 0;
    response.DistributionCount2 = 0;

    SendPacket(response.Write());
}

void WorldSession::HandleCanRedeemWowTokenForBalance(WorldPackets::Token::CanRedeemWowTokenForBalance& packet)
{
    WorldPackets::Token::WowTokenCanRedeemForBalanceResult response;
    response.UnkInt = packet.UnkInt;
    response.Result = sWorld->getBoolConfig(CONFIG_WOW_TOKEN_ENABLED) ? TOKEN_RESULT_SUCCESS : TOKEN_RESULT_ERROR_DISABLED;
    SendPacket(response.Write());
}