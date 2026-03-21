#pragma once

#include "models/persona_face_contracts.hpp"

// Forward declaration conservadora para não inundar test-suite
namespace v1 {
    class LGFX;
}
class LGFX_Sprite;

namespace ncos::services::persona_face {

class PersonaGraphicsAdapter {
 public:
  PersonaGraphicsAdapter();
  ~PersonaGraphicsAdapter();

  /**
   * @brief Acopla a TELA Real. 
   * Pre-aloca os Sprites de forma irreversível e estática na SRAM garantindo O(1) de custo Heap ao longo de anos de uso ininterrupto.
   */
  void bind_display(v1::LGFX* lcd);

  /**
   * @brief Otimização Extrema (Capricho): 
   * Traduz a anatomia POD purista Lote 1 para comandos de desenho em um sub-recorte SPI.
   */
  void render_frame(const PersonaRenderPayload& payload);

 private:
  v1::LGFX* lcd_;
  LGFX_Sprite* left_sprite_;
  LGFX_Sprite* right_sprite_;
  
  // Limites conservadores para englobar totalmente animações STARTLE
  // sem vazar pro framebuffer do O.S. (14 KB alloc/each).
  static constexpr int kSpriteWidth = 86;
  static constexpr int kSpriteHeight = 110;

  PersonaEyeSpec last_left_;
  PersonaEyeSpec last_right_;

  void draw_eye_to_sprite(LGFX_Sprite* spr, const PersonaEyeSpec& spec) const;
  void draw_masks(LGFX_Sprite* spr, const PersonaEyeSpec& spec) const;
  void handle_full_clear() const;
};

} // namespace ncos::services::persona_face
