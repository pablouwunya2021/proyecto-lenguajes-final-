// ============================================================
//  qeqchi_translator.hpp
//  Traduce oraciones Q'eqchi' al español
//
//  Usa un diccionario basado en el Vocabulario Q'eqchi'
//  (Academia de Lenguas Mayas de Guatemala, 2004).
//
//  Proceso:
//  1. Tokeniza la oración Q'eqchi' palabra por palabra
//  2. Busca cada palabra en el diccionario
//  3. Aplica reglas de orden VSO → SVO para el español
//  4. Devuelve la traducción más cercana posible
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace natural {

struct TranslationResult {
    std::string original;     // Texto Q'eqchi' original
    std::string spanish;      // Traducción al español
    std::vector<std::pair<std::string,std::string>> wordMap; // palabra→traducción
    bool        complete;     // true si se tradujo todo
    std::string notes;        // notas gramaticales
};

class QeqchiTranslator {
public:
    QeqchiTranslator();

    // Traduce una oración o texto completo
    TranslationResult translate(const std::string& text) const;

    // Traduce una sola palabra
    std::string translateWord(const std::string& word) const;

    // Serialización JSON para el API
    std::string toJSON(const TranslationResult& result) const;

private:
    // Diccionario Q'eqchi' → español
    std::unordered_map<std::string, std::string> dict_;

    void buildDictionary();

    // Tokeniza el texto en palabras
    std::vector<std::string> tokenize(const std::string& text) const;

    // Reordena de VSO a SVO para español más natural
    std::string reorderVSOtoSVO(const std::vector<std::string>& words) const;
};

} // namespace natural
