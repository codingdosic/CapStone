
#include "ChatWidget.h"

#include "Components/EditableTextBox.h"
#include <Components/TextBlock.h>

void UChatWidget::FocusOnInput()
{
    if (ChatInputBox)
    {
        // 한 프레임 지연 후 포커스 주기
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
            {
                ChatInputBox->SetKeyboardFocus();
            }, 0.01f, false); // 약간의 지연을 줌
    }
}

void UChatWidget::AddChatMessage(const FString& SenderId, const FString& Message)
{
    if (ChatScrollBox)
    {
        UTextBlock* NewMessage = NewObject<UTextBlock>(ChatScrollBox);
        if (NewMessage)
        {
            // SenderId의 앞 4글자 추출
            FString ShortId = SenderId.Left(4);

            // 메시지 형식 지정: "abcd: 메시지"
            FString FullMessage = FString::Printf(TEXT("%s: %s"), *ShortId, *Message);

            // 텍스트 설정 및 추가
            NewMessage->SetText(FText::FromString(FullMessage));
            ChatScrollBox->AddChild(NewMessage);
        }
    }
}



void UChatWidget::OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    UE_LOG(LogTemp, Warning, TEXT("OnChatTextCommitted Called! Commit Method: %d"), (int)CommitMethod);

    if (CommitMethod == ETextCommit::OnEnter)
    {
        if (OwnerCharacter && !Text.IsEmpty())
        {
            // 캐릭터에 메시지 전송 요청
            OwnerCharacter->SendChatMessage(Text.ToString());

            // 스크롤박스에 메시지 추가 (내 메시지)
            if (ChatScrollBox)
            {
                UTextBlock* NewTextBlock = NewObject<UTextBlock>(this);
                if (NewTextBlock)
                {
                    // 내 ID 앞 4자리 추출
                    FString ShortId = OwnerCharacter->GetMySocketId().Left(4);

                    // "abcd: 메시지" 형식으로 출력
                    FString FormattedMessage = FString::Printf(TEXT("%s: %s"), *ShortId, *Text.ToString());
                    NewTextBlock->SetText(FText::FromString(FormattedMessage));
                    ChatScrollBox->AddChild(NewTextBlock);

                    // 스크롤 아래로 내리기
                    ChatScrollBox->ScrollToEnd();
                }
            }
        }

        // 텍스트박스 비우기
        ChatInputBox->SetText(FText::GetEmpty());

        // 엔터 누른 후도 계속 포커스 유지
        FocusOnInput();
    }
}

//void UChatWidget::NativeConstruct()
//{
//    Super::NativeConstruct();
//
//    if (ChatInputBox)
//    {
//        ChatInputBox->OnTextCommitted.AddDynamic(this, &UChatWidget::OnChatTextCommitted);
//    }
//}

void UChatWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (ChatInputBox)
    {
        ChatInputBox->OnTextCommitted.AddDynamic(this, &UChatWidget::OnChatTextCommitted);
    }
}


FReply UChatWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        APlayerController* PC = GetOwningPlayer();
        if (PC)
        {
            // 한 프레임 뒤에 강제로 포커스 제거 + 게임 모드 전환
            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([PC]()
            {
                FInputModeGameOnly GameOnly;
                PC->SetInputMode(GameOnly);
                PC->bShowMouseCursor = false;
                FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);

            }), 0.01f, false); // 1 프레임 지연

            return FReply::Handled();
        }
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
