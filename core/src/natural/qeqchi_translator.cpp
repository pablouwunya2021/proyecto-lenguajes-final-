// ============================================================
//  qeqchi_translator.cpp
//  Diccionario basado en: Vocabulario Q'eqchi'-Español
//  Academia de Lenguas Mayas de Guatemala, 2004
// ============================================================
#include "natural/qeqchi_translator.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace natural {

QeqchiTranslator::QeqchiTranslator() {
    buildDictionary();
}

void QeqchiTranslator::buildDictionary() {
    // ── Partículas y artículos ────────────────────────────────
    dict_["li"]         = "el/la";
    dict_["eb'"]        = "los/las";
    dict_["sa'"]        = "en/dentro de";
    dict_["chi"]        = "con/usando";
    dict_["re"]         = "para/de";
    dict_["ut"]         = "y";
    dict_["naq"]        = "que";
    dict_["jo'"]        = "como";
    dict_["aj"]         = "(masculino)";
    dict_["ink'a'"]     = "no";
    dict_["maak'a'"]    = "no hay";
    dict_["jun"]        = "un/una";
    dict_["naab'al"]    = "muchos/muchas";
    dict_["juneq"]      = "uno";
    dict_["jwal"]       = "muy";

    // ── Pronombres personales ─────────────────────────────────
    dict_["laa'in"]     = "yo";
    dict_["laa'at"]     = "tú";
    dict_["laa'e"]      = "él/ella";
    dict_["laa'o"]      = "nosotros";
    dict_["laa'ex"]     = "ustedes";
    dict_["eb'"]        = "ellos/ellas";

    // ── Verbos (con y sin prefijo aspectual) ─────────────────
    // x- = completivo (pasado), y- = incompletivo (presente)
    dict_["wan"]        = "hay/existe/está";
    dict_["wank"]       = "hay/existe/está";
    dict_["wankat"]     = "existe/está";
    dict_["k'anjel"]    = "trabajar";
    dict_["k'anjelak"]  = "trabaja";
    dict_["xk'anjel"]   = "trabajó";
    dict_["b'ank"]      = "hacer";
    dict_["b'anow"]     = "haciendo";
    dict_["xb'an"]      = "hizo";
    dict_["xik"]        = "fue/salió";
    dict_["xike"]       = "se fue";
    dict_["k'ul"]       = "llegar";
    dict_["k'ulak"]     = "llega";
    dict_["xk'ul"]      = "llegó";
    dict_["xk'ulunk"]   = "llegó";
    dict_["k'ira"]      = "curar/mejorar";
    dict_["xk'ira"]     = "se curó/mejoró";
    dict_["il"]         = "ver";
    dict_["iloq"]       = "viendo";
    dict_["xil"]        = "vio";
    dict_["yejak"]      = "enseñar";
    dict_["taw"]        = "encontrar";
    dict_["xtaw"]       = "encontró";
    dict_["loq'"]       = "comprar";
    dict_["xloq'"]      = "compró";
    dict_["k'ay"]       = "vender";
    dict_["xk'ay"]      = "vendió";
    dict_["k'e"]        = "dar";
    dict_["xk'e"]       = "dio";
    dict_["ab'i"]       = "escuchar/oír";
    dict_["xab'i"]      = "escuchó";
    dict_["xwab'i"]     = "yo escuché";
    dict_["tz'iib'"]    = "escribir";
    dict_["naw"]        = "saber/conocer";
    dict_["tawb'"]      = "encontrar";
    dict_["kam"]        = "morir";
    dict_["xkam"]       = "murió";
    dict_["chap"]       = "agarrar/capturar";
    dict_["xchap"]      = "agarró/capturó";
    dict_["yee"]        = "decir";
    dict_["xye"]        = "dijo";
    dict_["t'an"]       = "cortar/talar";
    dict_["xt'an"]      = "cortó/taló";
    dict_["t'up"]       = "reventar";
    dict_["xint'up"]    = "reventé";
    dict_["t'ane'"]     = "caerse";
    dict_["xt'ane'"]    = "se cayó";
    dict_["tz'ap"]      = "encerrar";
    dict_["xtz'ap"]     = "encerró";
    dict_["kalaaq"]     = "estar borracho";
    dict_["ooxch'iila"] = "nos regañó";
    dict_["b'esok"]     = "cortar el cabello";
    dict_["nab'esok"]   = "corta el cabello";
    dict_["numtajenaq"] = "tiene muchos";
    dict_["yookeb'"]    = "están";
    dict_["yook"]       = "está/están haciendo";
    dict_["nake'"]      = "ellos";

    // ── Sustantivos ───────────────────────────────────────────
    dict_["aatin"]       = "palabra";
    dict_["aatinob'aal"] = "idioma/lengua";
    dict_["tenamit"]     = "pueblo/municipio";
    dict_["k'aleb'aal"]  = "comunidad";
    dict_["kab'l"]       = "casa";
    dict_["nima'"]       = "río";
    dict_["ch'och'"]     = "tierra/suelo";
    dict_["tz'oleb'aal"] = "escuela";
    dict_["tzoleb'aal"]  = "escuela";
    dict_["k'utunel"]    = "maestro/profesor";
    dict_["poyanam"]     = "gente/personas";
    dict_["winq"]        = "persona/hombre";
    dict_["ixq"]         = "mujer";
    dict_["ch'ina'al"]   = "niño/niña";
    dict_["k'anjel"]     = "trabajo";
    dict_["tumin"]       = "dinero";
    dict_["wakax"]       = "ganado/vaca";
    dict_["tz'i'"]       = "perro";
    dict_["ixim"]        = "maíz";
    dict_["ha'"]         = "agua";
    dict_["kutan"]       = "día";
    dict_["saqenk"]      = "amanecer/mañana";
    dict_["ab'"]         = "hamaca";
    dict_["tib'"]        = "carne";
    dict_["am"]          = "araña";
    dict_["xjolom"]      = "su cabeza";
    dict_["b'e"]         = "camino";
    dict_["tzekeemq"]    = "comida";
    dict_["jul"]         = "hoyo";
    dict_["ixqa'al"]     = "niña";
    dict_["tz'alam"]     = "cárcel";
    dict_["elq'"]        = "ladrón";
    dict_["kaxlan"]      = "pollo/gallina";
    dict_["halaw"]       = "tepezcuintle";
    dict_["sulul"]       = "lodo";
    dict_["rab'"]        = "su hamaca";
    dict_["rikan"]       = "su tío";
    dict_["ranab'aj"]    = "su hermana";
    dict_["wiitz'in"]    = "mi hermano menor";
    dict_["inyuwa'"]     = "mi papá";
    dict_["linna'"]      = "mi madre";
    dict_["mama'a"]      = "anciano";
    dict_["wechan"]      = "mi vecino";

    // ── Nombres propios (municipios y personas) ───────────────
    dict_["kob'an"]      = "Cobán";
    dict_["karcha"]      = "Carchá";
    dict_["watemaal"]    = "Guatemala";
    dict_["belice"]      = "Belice";
    dict_["kalich"]      = "Carlos";
    dict_["xmar"]        = "María";
    dict_["b'ex"]        = "Sebastián";
    dict_["ku"]          = "Domingo";
    dict_["lu'"]         = "Pedro";
    dict_["ixchel"]      = "Ixchel";

    // ── Adjetivos ─────────────────────────────────────────────
    dict_["nim"]         = "grande";
    dict_["ch'in"]       = "pequeño";
    dict_["us"]          = "bueno";
    dict_["chaab'il"]    = "bueno/bonito";
    dict_["ch'ina'us"]   = "pequeño y bonito";
    dict_["jwal josq'"]  = "muy enojado";
    dict_["jwal nim"]    = "muy grande";
    dict_["jwal pix"]    = "muy tacaño";
    dict_["saasa"]       = "divertido";
    dict_["chaq'al"]     = "hermoso";

    // ── Partículas de tiempo ──────────────────────────────────
    dict_["anajwank"]    = "hoy";
    dict_["anaqwan"]     = "ahora/hoy";
    dict_["ewer"]        = "ayer";
    dict_["chihab'"]     = "ayer";
    dict_["oxej"]        = "pasado mañana";
    dict_["wulaj"]       = "mañana";
    dict_["junes"]       = "mañana";
    dict_["toj"]         = "todavía";
    dict_["ta"]          = "(partícula futura)";
    dict_["chik"]        = "ya";
    dict_["moko"]        = "todavía no";
    dict_["mayer"]       = "antes";
    dict_["junelik"]     = "siempre";
    dict_["xkaanb'al"]   = "mediodía";

    // ── Prefijos aspectuales frecuentes ──────────────────────
    dict_["x"]           = "(completivo)";
    dict_["y"]           = "(incompletivo)";
    dict_["t"]           = "(futuro)";

    // ── Interrogativos ────────────────────────────────────────
    dict_["ani"]         = "¿quién?/¿cómo?";
    dict_["aniat"]       = "¿quién eres?";
    dict_["b'ar"]        = "¿dónde?";
    dict_["k'aru"]       = "¿qué?";
    dict_["k'a'ru"]      = "¿qué?";
    dict_["chanru"]      = "¿cómo?";
    dict_["k'a'ut"]      = "¿por qué?";
    dict_["k'aut"]       = "¿por qué?";
    dict_["hoonal"]      = "¿cuándo?";

    // ── Verbos adicionales de los ejemplos reales ─────────────
    dict_["wa'ak"]       = "comer";
    dict_["xwa'ak"]      = "comió";
    dict_["xinwa'ak"]    = "yo comí";
    dict_["chaakuy"]     = "perdona/ten misericordia";
    dict_["chaak'uy"]    = "perdona/ten misericordia";
    dict_["sak'"]        = "pegar/golpear";
    dict_["xsak'"]       = "pegó/golpeó";
    dict_["chunchu"]     = "está sentado";
    dict_["chunla"]      = "siéntate";
    dict_["ninchunla"]   = "me siento";
    dict_["jork"]        = "rajar/cortar";
    dict_["xkinjor"]     = "yo rajé";
    dict_["musiq'ak"]    = "respirar";
    dict_["yookin"]      = "estoy";
    dict_["yookat"]      = "estás";
    dict_["yookeb"]      = "están";
    dict_["yookeb'"]     = "están";
    dict_["yoo"]         = "está";
    dict_["wankin"]      = "estoy";
    dict_["wanko"]       = "estamos";
    dict_["puch'"]       = "lavar";
    dict_["xpuch'"]      = "lavó";
    dict_["tz'apok"]     = "cerrar";
    dict_["oso'"]        = "acabar/terminar";
    dict_["pisk'ok"]     = "saltar";
    dict_["kipisk'ok"]   = "saltó";
    dict_["taqok"]       = "enviar/mandar";
    dict_["taqataqla"]   = "lo/la enviarán";
    dict_["tinextaqla"]  = "me mandarán";
    dict_["xintaw"]      = "me encontré";
    dict_["naak'utun"]   = "se ve/aparece";
    dict_["xril"]        = "vio";
    dict_["naril"]       = "mira";
    dict_["k'ehok"]      = "respetar";
    dict_["nink'e"]      = "yo respeto";
    dict_["k'ehok"]      = "respetando";
    dict_["kitane'"]     = "se estrelló/se cayó";
    dict_["ra"]          = "duele";
    dict_["numen"]       = "pase/pasa";
    dict_["numenik"]     = "pasar";
    dict_["aatenqank"]   = "atender";
    dict_["nakaweeka"]   = "¿cómo te sientes?";
    dict_["weeka"]       = "sentirse";
    dict_["tink'ayi"]    = "venderé";
    dict_["xko"]         = "fue";
    dict_["xqil"]        = "vimos";
    dict_["taqab'antioxi"] = "demos gracias";
    dict_["b'antioxink"] = "agradecer";
    dict_["wuulank"]     = "visitar";
    dict_["cholok"]      = "explicar";
    dict_["sikb'ank"]    = "buscar";
    dict_["xsikb'al"]    = "está buscando";
    dict_["naxik"]       = "va/anda";

    // ── Sustantivos de los ejemplos reales ────────────────────
    dict_["k'ab'a'"]     = "nombre";
    dict_["laak'ab'a'"]  = "tu nombre";
    dict_["link'ab'a'"]  = "mi nombre";
    dict_["ink'ab'a'"]   = "mi nombre";
    dict_["ochoch"]      = "casa";
    dict_["wochoch"]     = "mi casa";
    dict_["si'"]         = "leña";
    dict_["k'aam"]       = "cuerda/soga";
    dict_["tz'ib'leb'"]  = "lapicero/lápiz";
    dict_["lintz'ib'leb'"] = "mis lapiceros";
    dict_["hu"]          = "papel/cuaderno";
    dict_["linhu"]       = "mi cuaderno";
    dict_["laahu"]       = "tu cuaderno";
    dict_["ismal"]       = "cabello";
    dict_["wismal"]      = "cabello";
    dict_["lawismal"]    = "tu cabello";
    dict_["liwismal"]    = "su cabello";
    dict_["k'uula'al"]   = "bebé";
    dict_["qulaal"]      = "bebé/nena";
    dict_["ch'utam"]     = "reunión";
    dict_["eerilb'al"]   = "reunión/encuentro";
    dict_["xaman"]       = "fin de semana";
    dict_["seb'uk'al"]   = "olla de barro";
    dict_["kok'kab'"]    = "caramelos/dulces";
    dict_["b'eleb'aalch'iich'"]  = "carro/vehículo";
    dict_["lixb'eleb'aalch'iich'"] = "su carro";
    dict_["laab'eleb'aalch'iich'"] = "tu carro";
    dict_["b'aqlaqch'iich'"]     = "motocicleta/bicicleta";
    dict_["lixb'aqlaqch'iich'"]  = "su motocicleta";
    dict_["linb'aqlaqch'iich'"]  = "mi motocicleta";
    dict_["sosolch'iich'"]       = "avión";
    dict_["chunleb'aal"] = "silla/sillón";
    dict_["chunleb'meex"]= "pupitre";
    dict_["kawa'chin"]   = "señor/caballero";
    dict_["kana'chin"]   = "señora";
    dict_["rahom"]       = "amor";
    dict_["inrahom"]     = "mi amor";
    dict_["ch'inaxk'aal"]= "niña";
    dict_["al"]          = "muchacho/joven";
    dict_["tib'elwa"]    = "comida/cena";
    dict_["aq"]          = "ropa/tela";
    dict_["wamiw"]       = "amigo/amiga";
    dict_["na'aj"]       = "parcela/terreno";
    dict_["na'b'ej"]     = "mamá";
    dict_["yuwa'b'ej"]   = "papá";
    dict_["k'auxl"]      = "corazón/mente";
    dict_["punit"]       = "gorra/sombrero";
    dict_["lixpunit"]    = "su gorra";
    dict_["ik"]          = "caldo/chile";
    dict_["kaq"]         = "rojo/caliente";
    dict_["naleb'"]      = "comportamiento/educación";
    dict_["lix naleb'"]  = "su comportamiento";
    dict_["tib'el"]      = "cuerpo/gordura";
    dict_["q'eqitz'i'"]  = "perro negro";
    dict_["na'"]         = "mamá/madre";
    dict_["yuwa'"]       = "papá/padre";
    dict_["linna'"]      = "mi mamá";
    dict_["linyuwa'"]    = "mi papá";
    dict_["ajaw"]        = "Dios/Señor";
    dict_["qawa'"]       = "Señor/Dios";
    dict_["Qawa'"]       = "Señor/Dios";
    dict_["ch'ina"]      = "pequeño/a";
    dict_["ch'ina si'"]  = "niña";
    dict_["k'iche'"]     = "montaña/bosque";
    dict_["ajralch'och'"]= "parte de la tierra";
    dict_["b'anleb'aal"] = "hospital/clínica";
    dict_["junchik"]     = "compañero/a";
    dict_["chijunileb'"] = "todos";
    dict_["chijunilex"]  = "todos ustedes";
    dict_["junjunqeb"]   = "cada uno";
    dict_["komonil"]     = "comunidad/todos juntos";
    dict_["aatinob'aal"] = "idioma/lengua";
    dict_["waatin"]      = "mis palabras";

    // ── Frases posesivas compuestas ───────────────────────────
    dict_["linch'ool"]   = "mi corazón";
    dict_["inch'ool"]    = "mi corazón";
    dict_["lixch'ool"]   = "su corazón/alma";
    dict_["aach'ool"]    = "tu corazón";
    dict_["qach'ool"]    = "nuestro corazón";
    dict_["injolom"]     = "mi cabeza";
    dict_["linjolom"]    = "mi cabeza";
    dict_["laajolom"]    = "tu cabeza";
    dict_["inmaak"]      = "mi pecado";
    dict_["ink'anjel"]   = "mi trabajo";
    dict_["laames"]      = "tu gato";
    dict_["linmes"]      = "mi gato";

    // ── Adjetivos de los ejemplos ─────────────────────────────
    dict_["sa"]          = "rico/delicioso";
    dict_["aanil"]       = "rápido";
    dict_["yib'"]        = "feo";
    dict_["mama'"]       = "viejo/anciano";
    dict_["puqul"]       = "pinchado/reventado";
    dict_["pomb'il"]     = "asado";
    dict_["k'ach'in"]    = "muy pequeño/chiquito";
    dict_["naj"]         = "alto";
    dict_["ch'inaus"]    = "qué lindo/bonito";
    dict_["b'antiox"]    = "gracias";
    dict_["B'antiox"]    = "gracias";
    dict_["b'anyox"]     = "gracias";
    dict_["laatz'in"]    = "estoy ocupado/a";
    dict_["seb'esinb'il"]= "espantado/asustado";

    // ── Partículas adicionales ────────────────────────────────
    dict_["rik'in"]      = "con";
    dict_["ruk'"]        = "con";
    dict_["wi'"]         = "también";
    dict_["ab'an"]       = "pero";
    dict_["ab'anan"]     = "sin embargo";
    dict_["tento"]       = "si/para que";
    dict_["ma"]          = "(interrogativo)";
    dict_["aawe"]        = "de ti/por ti/tuyo";
    dict_["aaw"]         = "tuyo";
    dict_["inkinawuulani"] = "me visitaste";
    dict_["inkinawuulan"]  = "me visitaste";
    dict_["xkaanb'al"]   = "al mediodía";
    dict_["chihab'"]     = "ayer";
    dict_["lixnaql"]     = "sus ojos";
    dict_["xnaqliru"]    = "a través de sus ojos";
    dict_["lixch'ool"]   = "su alma/corazón";
    dict_["sa'"]         = "en/dentro de";
    dict_["ajk'anjel"]   = "empleado/trabajador";
    dict_["ajralch'och'"] = "parte de la madre tierra";
    dict_["nintaw"]       = "encontré";
    dict_["wib'"]         = "dos";
    dict_["jumpaat"]      = "veloz/rápido";
    dict_["nin"]          = "(marcador 1a persona)";
    dict_["nink'e"]       = "yo respeto/doy";
    dict_["ninko"]        = "no tengo";
    dict_["seb'esinb'il"] = "espantado/asustado";
    dict_["Ra"]           = "duele";
    dict_["laatz'in"]     = "estoy ocupado/a";
    dict_["ok"]          = "voy a";
    dict_["we"]          = "(condicional)";
    dict_["b'ab'ay"]     = "un poco";
    dict_["jumpaat"]     = "rápido/veloz";
    dict_["nab'al"]      = "mucho/muchas";
    dict_["xtz'aq"]      = "vale/tiene valor";
    dict_["mak'a"]       = "no tiene/sin";
    dict_["ninko"]       = "no tengo";
    dict_["sa'"]         = "en/dentro de";
}

// ════════════════════════════════════════════════════════════
//  tokenize — divide el texto en palabras
// ════════════════════════════════════════════════════════════
std::vector<std::string> QeqchiTranslator::tokenize(const std::string& text) const {
    std::vector<std::string> words;
    std::string current;

    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        // Los apostrofes son parte de las palabras Q'eqchi' (marcadores glotales)
        if (std::isalpha((unsigned char)c) || c == '\'' || c == '\xc3') {
            current += c;
        } else if (!current.empty()) {
            words.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) words.push_back(current);
    return words;
}

// ════════════════════════════════════════════════════════════
//  translateWord — busca una palabra en el diccionario
//  Si no la encuentra, devuelve la palabra entre corchetes
// ════════════════════════════════════════════════════════════
std::string QeqchiTranslator::translateWord(const std::string& word) const {
    // Buscar exacto
    auto it = dict_.find(word);
    if (it != dict_.end()) return it->second;

    // Buscar en minúsculas
    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    it = dict_.find(lower);
    if (it != dict_.end()) return it->second;

    // No encontrada
    return "[" + word + "]";
}

// ════════════════════════════════════════════════════════════
//  translate — traduce texto completo con notas gramaticales
// ════════════════════════════════════════════════════════════
TranslationResult QeqchiTranslator::translate(const std::string& text) const {
    TranslationResult result;
    result.original = text;
    result.complete  = true;

    // Dividir en oraciones por punto
    std::vector<std::string> sentences;
    std::string current;
    for (char c : text) {
        if (c == '.') {
            if (!current.empty()) { sentences.push_back(current); current.clear(); }
        } else {
            current += c;
        }
    }
    if (!current.empty()) sentences.push_back(current);

    std::string fullTranslation;

    for (size_t si = 0; si < sentences.size(); si++) {
        const auto& sentence = sentences[si];
        auto words = tokenize(sentence);
        if (words.empty()) continue;

        // Traducir cada palabra
        std::vector<std::string> translated;
        bool allFound = true;

        for (const auto& w : words) {
            std::string trans = translateWord(w);
            result.wordMap.push_back({w, trans});

            // Si está entre corchetes no fue encontrada
            if (trans[0] == '[') allFound = false;
            translated.push_back(trans);
        }

        if (!allFound) result.complete = false;

        // Construir traducción de la oración
        // Q'eqchi' es VSO, español es SVO.
        // Heurística: si el primer token es un verbo existencial
        // (wan/wank), la estructura es Verbo + Sujeto + Locativo
        // → en español: "Hay [sujeto] [en locativo]"
        std::string sentenceTrans;
        for (size_t i = 0; i < translated.size(); i++) {
            if (i > 0) sentenceTrans += " ";
            // Omitir partículas aspectuales sueltas en la traducción visible
            if (translated[i] == "(completivo)" ||
                translated[i] == "(incompletivo)" ||
                translated[i] == "(futuro)") continue;
            sentenceTrans += translated[i];
        }

        // Limpiar espacios dobles
        std::string cleaned;
        bool prevSpace = false;
        for (char c : sentenceTrans) {
            if (c == ' ') {
                if (!prevSpace) cleaned += c;
                prevSpace = true;
            } else {
                cleaned += c;
                prevSpace = false;
            }
        }

        if (si > 0 && !fullTranslation.empty()) fullTranslation += " ";
        fullTranslation += cleaned;
        if (sentences.size() > 1) fullTranslation += ".";
    }

    result.spanish = fullTranslation;

    // Nota gramatical
    result.notes = "Q'eqchi' (Alta Verapaz, Guatemala). "
                   "Orden gramatical: Verbo-Sujeto-Objeto (VSO). "
                   "Prefijo x- = aspecto completivo (pasado). "
                   "li = articulo determinado.";

    return result;
}

// ════════════════════════════════════════════════════════════
//  toJSON — serializa para el API/frontend
// ════════════════════════════════════════════════════════════
std::string QeqchiTranslator::toJSON(const TranslationResult& result) const {
    auto esc = [](const std::string& s) {
        std::string r;
        for (char c : s) {
            if (c == '"')  r += "\\\"";
            else if (c == '\\') r += "\\\\";
            else if (c == '\n') r += "\\n";
            else r += c;
        }
        return r;
    };

    std::ostringstream j;
    j << "{\n"
      << "  \"original\": \""  << esc(result.original) << "\",\n"
      << "  \"spanish\": \""   << esc(result.spanish)  << "\",\n"
      << "  \"complete\": "    << (result.complete ? "true" : "false") << ",\n"
      << "  \"notes\": \""     << esc(result.notes)    << "\",\n"
      << "  \"word_map\": [\n";

    for (size_t i = 0; i < result.wordMap.size(); i++) {
        j << "    {\"qeqchi\": \"" << esc(result.wordMap[i].first)
          << "\", \"spanish\": \"" << esc(result.wordMap[i].second) << "\"}";
        if (i + 1 < result.wordMap.size()) j << ",";
        j << "\n";
    }
    j << "  ]\n}";
    return j.str();
}

} // namespace natural
