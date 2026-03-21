#include "persona_graphics_adapter.hpp"
#include <LovyanGFX.hpp>

namespace ncos::services::persona_face {

PersonaGraphicsAdapter::PersonaGraphicsAdapter() 
    : lcd_(nullptr), left_sprite_(nullptr), right_sprite_(nullptr) {}

PersonaGraphicsAdapter::~PersonaGraphicsAdapter() {
    if (left_sprite_) { left_sprite_->deleteSprite(); delete left_sprite_; }
    if (right_sprite_) { right_sprite_->deleteSprite(); delete right_sprite_; }
}

void PersonaGraphicsAdapter::bind_display(v1::LGFX* lcd) {
    lcd_ = lcd;
    
    // OTIMIZAÇÃO EXTREMA: Eliminação de Instanciação Transitória
    // Criar/Deletar objetos no freeRTOS gera fragmentation a longo prazo.
    // Assumiremos permanentemente 2 blocos restritos (SRAM estrita - 16bits color) de apenas 86x110px de impacto no frame.
    // Isso equivale a ~18KB cada. O S3 engole perfeitamente sem necessidade de PSRAM externa perigosa.
    if (!left_sprite_) {
        left_sprite_ = new LGFX_Sprite(lcd_);
        left_sprite_->setColorDepth(16);
        left_sprite_->createSprite(kSpriteWidth, kSpriteHeight); 
    }
    
    if (!right_sprite_) {
        right_sprite_ = new LGFX_Sprite(lcd_);
        right_sprite_->setColorDepth(16);
        right_sprite_->createSprite(kSpriteWidth, kSpriteHeight);
    }
}

void PersonaGraphicsAdapter::handle_full_clear() const {
    if (lcd_) {
        lcd_->fillScreen(TFT_BLACK);
    }
}

void PersonaGraphicsAdapter::render_frame(const PersonaRenderPayload& payload) {
    if (!lcd_ || !payload.frame_is_dirty) return;

    if (payload.force_full_black_clear) {
        handle_full_clear();
    }

    // Em vez de limpar e recriar, apenas enchemos de preto. (Zero malloc overhead O(1))
    left_sprite_->fillSprite(TFT_BLACK);
    right_sprite_->fillSprite(TFT_BLACK);

    draw_eye_to_sprite(left_sprite_, payload.left_current);
    draw_eye_to_sprite(right_sprite_, payload.right_current);

    draw_masks(left_sprite_, payload.left_current);
    draw_masks(right_sprite_, payload.right_current);

    // Bounding Box Flush:
    // Só envianos via DMA/SPI a sub-fatia de pixels que contém a geometria do olho, ignorando o vão negro de 320x240 LCD.
    // O centro do Sprite repousa na coordenada base calculada pelo Lote 2 Animator.
    int lx = payload.left_current.x - (kSpriteWidth / 2);
    int ly = payload.left_current.y - (kSpriteHeight / 2);
    
    int rx = payload.right_current.x - (kSpriteWidth / 2);
    int ry = payload.right_current.y - (kSpriteHeight / 2);

    lcd_->startWrite();
    left_sprite_->pushSprite(lx, ly);
    right_sprite_->pushSprite(rx, ry);
    lcd_->endWrite();

    last_left_ = payload.left_current;
    last_right_ = payload.right_current;
}

void PersonaGraphicsAdapter::draw_eye_to_sprite(LGFX_Sprite* spr, const PersonaEyeSpec& spec) const {
    if (!spec.active) return;

    // Compensação relativa: Dentro do sprite(86x110), o olho reside visualmente no centro (43, 55)
    int local_cx = kSpriteWidth / 2;
    int local_cy = kSpriteHeight / 2;

    spr->fillRoundRect(
        local_cx - (spec.width / 2),
        local_cy - (spec.height / 2),
        spec.width,
        spec.height,
        spec.radius,
        TFT_WHITE
    );
}

void PersonaGraphicsAdapter::draw_masks(LGFX_Sprite* spr, const PersonaEyeSpec& spec) const {
    int local_cx = kSpriteWidth / 2;
    int local_cy = kSpriteHeight / 2;

    // Máscara Oclusora angular para "Moon Crescent" e Blink Suave s/ Artefatos Lineares
    // Triangular clipping - "Caprichado" pra não esquadrinhar mal em LCDs ST7789
    if (spec.mask_upper_tilt != 0) {
        spr->fillTriangle(
            local_cx - (kSpriteWidth / 2), local_cy - (spec.height / 2), 
            local_cx + (kSpriteWidth / 2), local_cy - (spec.height / 2) + spec.mask_upper_tilt,
            local_cx - (kSpriteWidth / 2), local_cy - (kSpriteHeight / 2), // Teto limite
            TFT_BLACK
        );
        spr->fillTriangle(
            local_cx + (kSpriteWidth / 2), local_cy - (spec.height / 2) + spec.mask_upper_tilt,
            local_cx + (kSpriteWidth / 2), local_cy - (kSpriteHeight / 2),
            local_cx - (kSpriteWidth / 2), local_cy - (kSpriteHeight / 2),
            TFT_BLACK
        );
    }
    
    if (spec.mask_lower_tilt != 0) {
        spr->fillTriangle(
            local_cx - (kSpriteWidth / 2), local_cy + (spec.height / 2), 
            local_cx + (kSpriteWidth / 2), local_cy + (spec.height / 2) - spec.mask_lower_tilt,
            local_cx - (kSpriteWidth / 2), local_cy + (kSpriteHeight / 2), // Assoalho
            TFT_BLACK
        );
        spr->fillTriangle(
            local_cx + (kSpriteWidth / 2), local_cy + (spec.height / 2) - spec.mask_lower_tilt,
            local_cx + (kSpriteWidth / 2), local_cy + (kSpriteHeight / 2),
            local_cx - (kSpriteWidth / 2), local_cy + (kSpriteHeight / 2),
            TFT_BLACK
        );
    }
}

} // namespace ncos::services::persona_face
