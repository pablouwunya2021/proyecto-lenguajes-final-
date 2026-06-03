"""
Genera el PDF de documentación del proyecto Lenguajes Formales.
"""
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import cm
from reportlab.lib import colors
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, PageBreak,
    Table, TableStyle, HRFlowable, KeepTogether
)
from reportlab.lib.enums import TA_CENTER, TA_LEFT, TA_JUSTIFY

OUTPUT = "/Users/pablocabrera/lenguajes/proyecto-lenguajes-final-/Documentacion_Proyecto_Lenguajes.pdf"

# ── Paleta de colores ────────────────────────────────────────────────────────
DARK_BG   = colors.HexColor("#1e1e2e")
ACCENT    = colors.HexColor("#89b4fa")   # azul claro (catppuccin blue)
GREEN     = colors.HexColor("#a6e3a1")
YELLOW    = colors.HexColor("#f9e2af")
MAUVE     = colors.HexColor("#cba6f7")
RED       = colors.HexColor("#f38ba8")
TEAL      = colors.HexColor("#94e2d5")
TEXT      = colors.HexColor("#cdd6f4")
SUBTEXT   = colors.HexColor("#a6adc8")
SURFACE   = colors.HexColor("#313244")
OVERLAY   = colors.HexColor("#45475a")
WHITE     = colors.white
BLACK     = colors.black

# ── Estilos ──────────────────────────────────────────────────────────────────
base = getSampleStyleSheet()

def style(name, parent="Normal", **kw):
    s = ParagraphStyle(name, parent=base[parent], **kw)
    return s

S = {
    "cover_title": style("cover_title", "Title",
        fontSize=32, leading=40, textColor=ACCENT,
        alignment=TA_CENTER, spaceAfter=10),
    "cover_sub": style("cover_sub",
        fontSize=15, leading=22, textColor=TEXT,
        alignment=TA_CENTER, spaceAfter=6),
    "cover_meta": style("cover_meta",
        fontSize=11, leading=16, textColor=SUBTEXT,
        alignment=TA_CENTER, spaceAfter=4),
    "h1": style("h1", "Heading1",
        fontSize=20, leading=26, textColor=ACCENT,
        spaceBefore=18, spaceAfter=8, borderPad=4),
    "h2": style("h2", "Heading2",
        fontSize=14, leading=20, textColor=GREEN,
        spaceBefore=14, spaceAfter=6),
    "h3": style("h3", "Heading3",
        fontSize=12, leading=18, textColor=YELLOW,
        spaceBefore=10, spaceAfter=4),
    "body": style("body",
        fontSize=10, leading=15, textColor=BLACK,
        alignment=TA_JUSTIFY, spaceAfter=6),
    "bullet": style("bullet",
        fontSize=10, leading=14, textColor=BLACK,
        leftIndent=18, spaceAfter=3,
        bulletIndent=6, bulletText="•"),
    "code": style("code",
        fontName="Courier", fontSize=8, leading=12,
        textColor=colors.HexColor("#3d3d3d"),
        backColor=colors.HexColor("#f5f5f5"),
        leftIndent=12, rightIndent=12,
        spaceAfter=8, spaceBefore=4,
        borderPad=6, borderColor=colors.HexColor("#cccccc"),
        borderWidth=0.5),
    "code_label": style("code_label",
        fontSize=8, leading=12, textColor=SUBTEXT,
        fontName="Courier-Oblique", spaceAfter=2),
    "caption": style("caption",
        fontSize=9, leading=13, textColor=SUBTEXT,
        alignment=TA_CENTER, spaceAfter=8),
    "note": style("note",
        fontSize=9, leading=13, textColor=colors.HexColor("#555555"),
        leftIndent=12, rightIndent=12, backColor=colors.HexColor("#fff9e6"),
        borderPad=5, borderColor=YELLOW, borderWidth=0.5, spaceAfter=8),
    "toc_title": style("toc_title",
        fontSize=16, leading=22, textColor=ACCENT,
        spaceBefore=0, spaceAfter=12, alignment=TA_CENTER),
    "toc_entry": style("toc_entry",
        fontSize=11, leading=18, textColor=BLACK),
    "toc_entry_sub": style("toc_entry_sub",
        fontSize=10, leading=16, textColor=colors.HexColor("#444444"),
        leftIndent=20),
}

# ── Helper: bloque de código con etiqueta ────────────────────────────────────
def code_block(label, code_text):
    items = []
    if label:
        items.append(Paragraph(f"// {label}", S["code_label"]))
    # Escapar caracteres especiales para XML de ReportLab
    safe = (code_text
            .replace("&", "&amp;")
            .replace("<", "&lt;")
            .replace(">", "&gt;"))
    items.append(Paragraph(safe, S["code"]))
    return items

def section_rule(color=ACCENT):
    return HRFlowable(width="100%", thickness=1.5, color=color, spaceAfter=8, spaceBefore=4)

def box_table(content_paragraphs, bg=colors.HexColor("#f0f4ff"), border=ACCENT):
    """Envuelve parrafos en una tabla con fondo de color."""
    data = [[p] for p in content_paragraphs]
    t = Table(data, colWidths=[16*cm])
    t.setStyle(TableStyle([
        ("BACKGROUND", (0,0), (-1,-1), bg),
        ("LINEABOVE",  (0,0), (-1,0), 1.5, border),
        ("LINEBELOW",  (0,-1), (-1,-1), 1.5, border),
        ("LINEBEFORE", (0,0), (0,-1), 1.5, border),
        ("LINEAFTER",  (-1,0), (-1,-1), 1.5, border),
        ("TOPPADDING",    (0,0), (-1,-1), 8),
        ("BOTTOMPADDING", (0,0), (-1,-1), 8),
        ("LEFTPADDING",   (0,0), (-1,-1), 10),
        ("RIGHTPADDING",  (0,0), (-1,-1), 10),
    ]))
    return t

# ── Construcción del documento ───────────────────────────────────────────────
def build():
    doc = SimpleDocTemplate(
        OUTPUT,
        pagesize=A4,
        leftMargin=2.5*cm, rightMargin=2.5*cm,
        topMargin=2.5*cm, bottomMargin=2.5*cm,
        title="Documentación Proyecto Lenguajes Formales",
        author="pablo"
    )

    story = []

    # ════════════════════════════════════════════════════════
    # PORTADA
    # ════════════════════════════════════════════════════════
    story += [
        Spacer(1, 3*cm),
        Paragraph("Compilador Web Interactivo", S["cover_title"]),
        Spacer(1, 0.3*cm),
        Paragraph("YALex + YAPar — Documentación Técnica Completa", S["cover_sub"]),
        Spacer(1, 0.5*cm),
        section_rule(ACCENT),
        Spacer(1, 0.3*cm),
        Paragraph("Curso: Lenguajes Formales y de Programación", S["cover_meta"]),
        Paragraph("Stack: C++17 · Python/FastAPI · React/TypeScript", S["cover_meta"]),
        Paragraph("Junio 2026", S["cover_meta"]),
        Spacer(1, 4*cm),
        box_table([
            Paragraph("<b>Resumen del Proyecto</b>", style("bsum", fontSize=11, textColor=ACCENT, spaceAfter=4)),
            Paragraph(
                "Este proyecto implementa un <b>IDE web completo</b> para el análisis léxico y sintáctico "
                "de lenguajes formales. El núcleo está escrito en C++17 y construye autómatas finitos, "
                "tablas de parsing SLR/LALR/LL(1) y árboles sintácticos desde cero. Una API FastAPI "
                "expone el motor C++ al mundo web, y un frontend React permite editar, analizar y "
                "visualizar todo en tiempo real desde el navegador.",
                style("bsumb", fontSize=10, leading=15, textColor=BLACK)),
        ], bg=colors.HexColor("#eef4ff"), border=ACCENT),
        PageBreak(),
    ]

    # ════════════════════════════════════════════════════════
    # TABLA DE CONTENIDOS
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("Tabla de Contenidos", S["toc_title"]),
        section_rule(ACCENT),
        Spacer(1, 0.3*cm),
    ]

    toc_entries = [
        ("1.", "¿Qué es este proyecto? — Visión general", False),
        ("2.", "Arquitectura del sistema", False),
        ("3.", "Módulo YALex — Análisis Léxico", False),
        ("   3.1", "¿Qué es el análisis léxico?", True),
        ("   3.2", "Lectura del archivo .yalex", True),
        ("   3.3", "Motor de expresiones regulares", True),
        ("   3.4", "Construcción del NFA (Thompson)", True),
        ("   3.5", "De NFA a DFA (Subconjuntos)", True),
        ("   3.6", "Minimización del DFA (Hopcroft)", True),
        ("   3.7", "Tokenizador final", True),
        ("4.", "Módulo YAPar — Análisis Sintáctico", False),
        ("   4.1", "¿Qué es el análisis sintáctico?", True),
        ("   4.2", "Carga y validación de la gramática", True),
        ("   4.3", "Conjuntos FIRST y FOLLOW", True),
        ("   4.4", "Autómata LR(0)", True),
        ("   4.5", "Tabla SLR(1)", True),
        ("   4.6", "Tabla LALR(1)", True),
        ("   4.7", "Tabla LL(1)", True),
        ("   4.8", "Parser (análisis bottom-up)", True),
        ("   4.9", "Resolución de conflictos", True),
        ("5.", "API REST (Python / FastAPI)", False),
        ("6.", "Frontend (React / TypeScript)", False),
        ("7.", "Flujo completo de datos — ejemplo paso a paso", False),
        ("8.", "Formatos de entrada: .yalex y .yapar", False),
        ("9.", "Resumen y conclusiones", False),
    ]

    for num, title, sub in toc_entries:
        s = S["toc_entry_sub"] if sub else S["toc_entry"]
        story.append(Paragraph(f"{num}  {title}", s))

    story.append(PageBreak())

    # ════════════════════════════════════════════════════════
    # 1. VISIÓN GENERAL
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("1. ¿Qué es este proyecto?", S["h1"]),
        section_rule(),
        Paragraph(
            "Este proyecto es un <b>compilador web interactivo</b>. Su propósito es permitir "
            "que alguien defina su propio lenguaje de programación (o cualquier lenguaje formal) "
            "a través de dos archivos de especificación, y luego analice texto de entrada "
            "para ver si es válido en ese lenguaje, qué tokens contiene y cómo se parsea.",
            S["body"]),
        Spacer(1, 0.2*cm),
        Paragraph(
            "Para entenderlo con una analogía: si quisieras crear el lenguaje <b>Q'eqchi'Script</b>, "
            "primero describirías cuáles son sus palabras clave y patrones de texto (análisis léxico) "
            "y luego definirías las reglas gramaticales de cómo se combinan (análisis sintáctico). "
            "Este proyecto hace exactamente eso de forma automática.",
            S["body"]),
        Spacer(1, 0.3*cm),
        Paragraph("¿Para qué sirve concretamente?", S["h3"]),
        Paragraph("Dado un archivo <b>.yalex</b> (especificación léxica) y un archivo <b>.yapar</b> "
                  "(especificación gramatical), el sistema puede:", S["body"]),
    ]

    bullets_general = [
        "Reconocer automáticamente los tokens (palabras) de cualquier texto de entrada.",
        "Construir el autómata finito determinista (DFA) que reconoce esos tokens.",
        "Construir las tablas de parsing (SLR, LALR, LL1) para la gramática.",
        "Validar si un texto respeta la gramática y generar el árbol sintáctico.",
        "Visualizar todo en un IDE web con editor de código, consola de errores y diagramas.",
    ]
    for b in bullets_general:
        story.append(Paragraph(b, S["bullet"]))
    story.append(Spacer(1, 0.4*cm))

    story += [
        Paragraph("Soporte de lenguajes especiales", S["h3"]),
        Paragraph(
            "El proyecto también incluye soporte experimental para lenguajes creativos como "
            "<b>Q'eqchi'</b> (lengua maya de Guatemala), <b>MessiScript</b>, <b>COW</b> y "
            "<b>OK COMPUTER</b>, con detección automática del tipo de archivo.",
            S["body"]),
        PageBreak(),
    ]

    # ════════════════════════════════════════════════════════
    # 2. ARQUITECTURA
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("2. Arquitectura del Sistema", S["h1"]),
        section_rule(),
        Paragraph(
            "El sistema tiene <b>tres capas</b> bien diferenciadas que se comunican entre sí:",
            S["body"]),
        Spacer(1, 0.3*cm),
    ]

    arch_data = [
        ["Capa", "Tecnología", "Responsabilidad"],
        ["Frontend", "React 18 + TypeScript\nVite + Monaco Editor", "IDE web: editores, visualizaciones, consola"],
        ["API", "Python 3 + FastAPI\nPydantic + CORS", "Recibe JSON del frontend, invoca los binarios C++"],
        ["Core", "C++17 + CMake\nnlohmann/json", "Toda la lógica: regex, NFA, DFA, gramática, tablas, parser"],
    ]

    arch_table = Table(arch_data, colWidths=[3.5*cm, 5*cm, 7.5*cm])
    arch_table.setStyle(TableStyle([
        ("BACKGROUND", (0,0), (-1,0), ACCENT),
        ("TEXTCOLOR",  (0,0), (-1,0), WHITE),
        ("FONTNAME",   (0,0), (-1,0), "Helvetica-Bold"),
        ("FONTSIZE",   (0,0), (-1,-1), 9),
        ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.HexColor("#f0f4ff"), WHITE]),
        ("GRID",       (0,0), (-1,-1), 0.5, colors.HexColor("#cccccc")),
        ("TOPPADDING",    (0,0), (-1,-1), 6),
        ("BOTTOMPADDING", (0,0), (-1,-1), 6),
        ("LEFTPADDING",   (0,0), (-1,-1), 8),
        ("VALIGN",        (0,0), (-1,-1), "TOP"),
    ]))
    story.append(arch_table)
    story.append(Spacer(1, 0.5*cm))

    story += [
        Paragraph("Flujo general de comunicación:", S["h3"]),
        Paragraph(
            "El usuario escribe o sube archivos en el <b>frontend</b>. Al hacer clic en "
            "\"Ejecutar\", el navegador envía una petición HTTP POST a la <b>API</b> en Python. "
            "La API guarda los contenidos en archivos temporales y ejecuta el binario C++ "
            "correspondiente (<code>yalex_cli</code> o <code>yapar_cli</code>) como subproceso. "
            "El binario devuelve un JSON por stdout que la API reenvía al frontend. "
            "El frontend parsea el JSON y actualiza todas las pestañas de visualización.",
            S["body"]),
        Spacer(1, 0.3*cm),
    ]

    # Diagrama ASCII como código
    diagram = (
        "  [Navegador / React]\n"
        "        |\n"
        "        |  HTTP POST /yalex/analyze\n"
        "        |  HTTP POST /yapar/analyze\n"
        "        v\n"
        "  [FastAPI Server - puerto 8000]\n"
        "        |\n"
        "        |  subprocess.run(yalex_cli, ...)\n"
        "        |  subprocess.run(yapar_cli, ...)\n"
        "        v\n"
        "  [Binario C++ compilado]\n"
        "        |\n"
        "        |  stdout: JSON con tokens/DFA/tablas/arbol\n"
        "        v\n"
        "  [FastAPI -> Frontend -> Visualizaciones]"
    )
    story += code_block("Flujo de datos", diagram)
    story.append(PageBreak())

    # ════════════════════════════════════════════════════════
    # 3. MÓDULO YALEX
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("3. Módulo YALex — Análisis Léxico", S["h1"]),
        section_rule(GREEN),
    ]

    # 3.1
    story += [
        Paragraph("3.1 ¿Qué es el análisis léxico?", S["h2"]),
        Paragraph(
            "El análisis léxico es el primer paso de un compilador. Su trabajo es leer "
            "una secuencia de caracteres y agruparlos en <b>tokens</b>: las unidades mínimas "
            "con significado. Por ejemplo, al analizar la expresión <code>x := 3 + y</code>, "
            "un analizador léxico produce los tokens: "
            "<code>ID(x)</code>, <code>ASSIGN</code>, <code>INT(3)</code>, "
            "<code>PLUS</code>, <code>ID(y)</code>.",
            S["body"]),
        Paragraph(
            "Para saber qué es un token, se usan <b>expresiones regulares</b>. Cada token "
            "se define como un patrón regex. Por ejemplo: <code>ID = [a-z][a-z0-9]*</code> "
            "significa que un identificador empieza con minúscula y puede seguir con letras "
            "o dígitos.",
            S["body"]),
        Spacer(1, 0.3*cm),
    ]

    # 3.2
    story += [
        Paragraph("3.2 Lectura del archivo .yalex", S["h2"]),
        Paragraph(
            "El archivo <b>.yalex</b> es la especificación léxica. Tiene dos partes: "
            "primero se definen <b>macros</b> (nombres para patrones reutilizables) y luego "
            "se definen las <b>reglas</b> que asocian cada patrón con un nombre de token.",
            S["body"]),
    ]

    yalex_example = (
        'let digit = ["0"-"9"]\n'
        'let letter = ["a"-"z"] | ["A"-"Z"]\n'
        'let id     = letter (letter | digit)*\n'
        '\n'
        'rule tokens =\n'
        '  digit+           { INT    }\n'
        '| id               { ID     }\n'
        '| "+"              { PLUS   }\n'
        '| "-"              { MINUS  }\n'
        '| " " | "\\t"      { ignore }\n'
    )
    story += code_block("Ejemplo de archivo .yalex", yalex_example)

    story += [
        Paragraph(
            "El módulo <code>yalex_reader.cpp</code> (899 líneas) lee este formato y también "
            "soporta variantes como <code>.yal</code>, <code>.lex</code> y <code>.l</code>. "
            "Extrae las macros y las reglas, y las pasa al motor de expresiones regulares.",
            S["body"]),
        Spacer(1, 0.3*cm),
    ]

    # 3.3
    story += [
        Paragraph("3.3 Motor de Expresiones Regulares", S["h2"]),
        Paragraph(
            "El archivo <code>regex_engine.cpp</code> convierte cada expresión regular en "
            "una forma que la máquina puede procesar. Lo hace en <b>tres pasos</b>:",
            S["body"]),
        Paragraph("<b>Paso 1 — Expansión de macros:</b> reemplaza las referencias "
                  "como <code>{digit}</code> por su definición real.", S["bullet"]),
        Paragraph("<b>Paso 2 — Expansión de clases de caracteres:</b> convierte "
                  "<code>[a-z]</code> en <code>(a|b|c|...|z)</code>.", S["bullet"]),
        Paragraph("<b>Paso 3 — Conversión a postfix (Shunting-Yard):</b> reorganiza los "
                  "operadores para que sea fácil construir el autómata.", S["bullet"]),
        Spacer(1, 0.2*cm),
    ]

    shunting = (
        "// El algoritmo Shunting-Yard convierte infijo a postfijo:\n"
        "// Entrada (infijo):  a | b * c\n"
        "// Salida (postfijo): a b c * |\n"
        "//\n"
        "// Prioridades de operadores:\n"
        "//   * + ?   (unarios, más alta)  -> precedencia 3\n"
        "//   concat  (implícito)          -> precedencia 2\n"
        "//   |       (alternativa)        -> precedencia 1\n"
        "\n"
        "int RegexParser::precedence(RegexTokenType type) {\n"
        "    switch (type) {\n"
        "        case KLEENE_STAR:\n"
        "        case PLUS:\n"
        "        case OPTIONAL: return 3;\n"
        "        case CONCAT:   return 2;\n"
        "        case UNION:    return 1;\n"
        "        default:       return 0;\n"
        "    }\n"
        "}"
    )
    story += code_block("regex_engine.cpp — precedencia de operadores", shunting)
    story.append(Spacer(1, 0.3*cm))

    # 3.4
    story += [
        Paragraph("3.4 Construcción del NFA (Algoritmo de Thompson)", S["h2"]),
        Paragraph(
            "Una vez que la regex está en notación postfix, se construye un "
            "<b>NFA (Non-deterministic Finite Automaton)</b> usando el "
            "<b>algoritmo de Thompson</b>. La idea es simple: cada operador regex "
            "se convierte en un pequeño fragmento de autómata, y los fragmentos "
            "se ensamblan:",
            S["body"]),
    ]

    thompson_rules = [
        ["Regex", "Fragmento NFA"],
        ["Literal 'a'", "Estado inicial → (con 'a') → Estado final"],
        ["A | B (unión)", "Nuevo inicio con ε a A y B; sus finales van a nuevo fin con ε"],
        ["AB (concat)", "El final de A se convierte en el inicio de B"],
        ["A* (Kleene)", "Nuevo inicio y fin; ε desde inicio a A-inicio y a nuevo fin; "
                        "ε desde A-fin a A-inicio y a nuevo fin"],
        ["A+ (plus)", "igual que A*, pero sin la ε directa inicio→fin"],
        ["A? (opcional)", "igual que A*, pero sin el bucle interno"],
    ]

    nfa_table = Table(thompson_rules, colWidths=[4*cm, 12*cm])
    nfa_table.setStyle(TableStyle([
        ("BACKGROUND", (0,0), (-1,0), GREEN),
        ("TEXTCOLOR",  (0,0), (-1,0), BLACK),
        ("FONTNAME",   (0,0), (-1,0), "Helvetica-Bold"),
        ("FONTSIZE",   (0,0), (-1,-1), 9),
        ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.HexColor("#f0fff4"), WHITE]),
        ("GRID",       (0,0), (-1,-1), 0.5, colors.HexColor("#cccccc")),
        ("TOPPADDING",    (0,0), (-1,-1), 5),
        ("BOTTOMPADDING", (0,0), (-1,-1), 5),
        ("LEFTPADDING",   (0,0), (-1,-1), 8),
        ("VALIGN",        (0,0), (-1,-1), "TOP"),
    ]))
    story.append(nfa_table)
    story.append(Spacer(1, 0.4*cm))

    # 3.5
    story += [
        Paragraph("3.5 De NFA a DFA — Construcción de Subconjuntos", S["h2"]),
        Paragraph(
            "Un NFA puede estar en varios estados a la vez (no-determinismo). "
            "Para poder simularlo eficientemente, se convierte en un "
            "<b>DFA (Deterministic Finite Automaton)</b> usando el algoritmo de "
            "<b>construcción de subconjuntos</b>:",
            S["body"]),
        Paragraph("Se calcula la <b>ε-clausura</b> del estado inicial del NFA "
                  "(todos los estados alcanzables sin consumir entrada).", S["bullet"]),
        Paragraph("Ese conjunto de estados del NFA se convierte en un <b>estado del DFA</b>.", S["bullet"]),
        Paragraph("Para cada símbolo posible, se calculan los estados NFA alcanzables "
                  "y su ε-clausura → nuevo estado DFA.", S["bullet"]),
        Paragraph("Se repite hasta no haber estados nuevos.", S["bullet"]),
        Spacer(1, 0.3*cm),
    ]

    # 3.6
    story += [
        Paragraph("3.6 Minimización del DFA — Algoritmo de Hopcroft", S["h2"]),
        Paragraph(
            "La construcción de subconjuntos puede generar estados DFA redundantes. "
            "El <b>algoritmo de Hopcroft</b> los minimiza agrupando estados "
            "equivalentes (estados que se comportan igual para toda entrada futura):",
            S["body"]),
        Paragraph("Se parte de dos grupos: estados de aceptación y no-aceptación.", S["bullet"]),
        Paragraph("Iterativamente se subdividen grupos: si dentro de un grupo dos estados "
                  "transicionan a grupos distintos para algún símbolo, se separan.", S["bullet"]),
        Paragraph("Cuando no hay más subdivisiones, cada grupo se convierte en "
                  "un estado del DFA mínimo.", S["bullet"]),
        Spacer(1, 0.3*cm),
    ]

    # 3.7
    story += [
        Paragraph("3.7 Tokenizador Final", S["h2"]),
        Paragraph(
            "El archivo <code>tokenizer.cpp</code> orquesta todo el proceso léxico. "
            "Combina los DFAs de todas las reglas del .yalex en uno solo (con prioridad: "
            "el token declarado primero gana en caso de empate). Luego recorre el texto "
            "de entrada y produce la lista de tokens usando simulación del DFA.",
            S["body"]),
        Spacer(1, 0.3*cm),
        Paragraph(
            "El resultado es un JSON con: lista de tokens encontrados, la "
            "representación del DFA (estados, transiciones), y un grafo en formato "
            "Graphviz DOT para visualizar el autómata.",
            S["body"]),
        PageBreak(),
    ]

    # ════════════════════════════════════════════════════════
    # 4. MÓDULO YAPAR
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("4. Módulo YAPar — Análisis Sintáctico", S["h1"]),
        section_rule(YELLOW),
    ]

    # 4.1
    story += [
        Paragraph("4.1 ¿Qué es el análisis sintáctico?", S["h2"]),
        Paragraph(
            "El análisis sintáctico (parsing) toma la lista de tokens producida por "
            "el analizador léxico y verifica que su secuencia respeta la <b>gramática</b> "
            "del lenguaje. Si es válida, produce un <b>árbol sintáctico</b> que representa "
            "la estructura jerárquica del programa.",
            S["body"]),
        Paragraph(
            "Las gramáticas se expresan en <b>BNF (Backus-Naur Form)</b>. Por ejemplo, "
            "una gramática simple para expresiones aritméticas:",
            S["body"]),
    ]

    grammar_ex = (
        "%token INT PLUS TIMES LPAREN RPAREN\n"
        "%start expr\n"
        "%%\n"
        "expr : expr PLUS term\n"
        "     | term\n"
        "     ;\n"
        "term : term TIMES factor\n"
        "     | factor\n"
        "     ;\n"
        "factor : LPAREN expr RPAREN\n"
        "       | INT\n"
        "       ;"
    )
    story += code_block("Ejemplo de archivo .yapar", grammar_ex)
    story.append(Spacer(1, 0.2*cm))

    # 4.2
    story += [
        Paragraph("4.2 Carga y Validación de la Gramática", S["h2"]),
        Paragraph(
            "El archivo <code>grammar.cpp</code> (603 líneas) lee el archivo .yapar. "
            "El formato soporta: declaración de tokens con <code>%token</code>, "
            "símbolo inicial con <code>%start</code>, asociatividad/precedencia con "
            "<code>%left</code>/<code>%right</code>/<code>%nonassoc</code>, y las "
            "reglas de producción tras el separador <code>%%</code>.",
            S["body"]),
        Paragraph(
            "Tras cargar, realiza <b>validaciones</b>: verifica que exista el separador "
            "<code>%%</code>, que haya al menos una producción, que el símbolo inicial "
            "esté definido, y que todos los no-terminales usados tengan sus producciones.",
            S["body"]),
        Paragraph(
            "Finalmente <b>aumenta la gramática</b> añadiendo una producción "
            "<code>S' → S</code> que es necesaria para construir las tablas SLR/LALR correctamente.",
            S["body"]),
        Spacer(1, 0.3*cm),
    ]

    augment_code = (
        "// Aumentar la gramática: S' → S\n"
        "// Es imprescindible para que el estado ACCEPT sea único\n"
        "if (!startSymbol_.name.empty() && !productions_.empty()) {\n"
        "    std::string augName = startSymbol_.name + \"'\";\n"
        "    Production aug;\n"
        "    aug.lhs = Symbol::nonTerminal(augName);\n"
        "    aug.rhs = { startSymbol_ };\n"
        "    productions_.insert(productions_.begin(), aug);\n"
        "    startSymbol_ = Symbol::nonTerminal(augName);\n"
        "}"
    )
    story += code_block("grammar.cpp — Gramática aumentada", augment_code)

    # 4.3
    story += [
        Paragraph("4.3 Conjuntos FIRST y FOLLOW", S["h2"]),
        Paragraph(
            "Para construir las tablas de parsing, primero hay que calcular dos conjuntos "
            "para cada símbolo de la gramática:",
            S["body"]),
        Paragraph(
            "<b>FIRST(A)</b>: el conjunto de terminales con los que puede comenzar "
            "cualquier cadena derivable de A. Si A puede derivar ε, también incluye ε.",
            S["bullet"]),
        Paragraph(
            "<b>FOLLOW(A)</b>: el conjunto de terminales que pueden aparecer "
            "inmediatamente después de A en alguna forma sentencial. El símbolo "
            "inicial siempre tiene $ (fin de entrada) en su FOLLOW.",
            S["bullet"]),
        Spacer(1, 0.2*cm),
        Paragraph(
            "Ambos se calculan con un algoritmo de <b>punto fijo</b>: se inicializan "
            "vacíos y se iteran las reglas hasta que no haya cambios.",
            S["body"]),
    ]

    first_code = (
        "// FIRST — algoritmo de punto fijo\n"
        "// Para cada producción A → X1 X2 ... Xn:\n"
        "//   Agrega FIRST(X1) - {ε} a FIRST(A)\n"
        "//   Si ε ∈ FIRST(X1): también agrega FIRST(X2) - {ε}\n"
        "//   ... y así hasta Xi donde ε ∉ FIRST(Xi)\n"
        "//   Si todos Xi tienen ε: agrega ε a FIRST(A)\n"
        "\n"
        "bool changed = true;\n"
        "while (changed) {\n"
        "    changed = false;\n"
        "    for (const auto& prod : productions_) {\n"
        "        bool allHaveEpsilon = true;\n"
        "        for (const auto& Xi : prod.rhs) {\n"
        "            for (const auto& f : first_[Xi.name])\n"
        "                if (f != EPSILON)\n"
        "                    changed |= first_[A].insert(f).second;\n"
        "            if (!first_[Xi.name].count(EPSILON)) {\n"
        "                allHaveEpsilon = false; break;\n"
        "            }\n"
        "        }\n"
        "        if (allHaveEpsilon)\n"
        "            changed |= first_[A].insert(EPSILON).second;\n"
        "    }\n"
        "}"
    )
    story += code_block("grammar.cpp — Cálculo de FIRST (punto fijo)", first_code)
    story.append(Spacer(1, 0.3*cm))

    # 4.4
    story += [
        Paragraph("4.4 Autómata LR(0)", S["h2"]),
        Paragraph(
            "Un <b>ítem LR(0)</b> es una producción con un punto (•) que indica "
            "hasta dónde se ha leído. Por ejemplo, <code>E → E • + T</code> "
            "significa que ya se leyó E y se espera ver + luego T.",
            S["body"]),
        Paragraph(
            "El <b>autómata LR(0)</b> tiene un estado por cada conjunto de ítems. "
            "Se construye con dos operaciones:",
            S["body"]),
        Paragraph(
            "<b>closure(I)</b>: si en el estado hay un ítem <code>A → α • B β</code>, "
            "se añaden también los ítems <code>B → • γ</code> para todas las "
            "producciones de B.", S["bullet"]),
        Paragraph(
            "<b>goto(I, X)</b>: nuevo estado formado por avanzar el punto sobre X "
            "en todos los ítems de I donde el símbolo tras el punto es X, "
            "y aplicar closure.", S["bullet"]),
        Spacer(1, 0.2*cm),
    ]

    closure_code = (
        "// closure(I): cierra un conjunto de ítems LR(0)\n"
        "// Si A → α • B β está en I y B es no-terminal,\n"
        "// añadimos todos los ítems B → • γ\n"
        "\n"
        "std::set&lt;LR0Item&gt; LR0Automaton::closure(const std::set&lt;LR0Item&gt;& items) {\n"
        "    std::set&lt;LR0Item&gt; result = items;\n"
        "    std::queue&lt;LR0Item&gt; worklist;\n"
        "    for (const auto& item : items) worklist.push(item);\n"
        "\n"
        "    while (!worklist.empty()) {\n"
        "        LR0Item current = worklist.front(); worklist.pop();\n"
        "        std::string B = symbolAfterDot(current);\n"
        "        if (B.empty()) continue;  // punto al final\n"
        "\n"
        "        // Agregar todos los items B → • gamma\n"
        "        for (int prodId : grammar_->getProductionsFor(B)) {\n"
        "            LR0Item newItem{prodId, 0};\n"
        "            if (!result.count(newItem)) {\n"
        "                result.insert(newItem);\n"
        "                worklist.push(newItem);\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    return result;\n"
        "}"
    )
    story += code_block("lr0_automaton.cpp — función closure", closure_code)
    story.append(Spacer(1, 0.3*cm))

    # 4.5 SLR
    story += [
        Paragraph("4.5 Tabla SLR(1)", S["h2"]),
        Paragraph(
            "La tabla <b>SLR(1)</b> (Simple LR) usa el autómata LR(0) + los conjuntos FOLLOW "
            "para decidir qué acción tomar en cada estado según el siguiente token:",
            S["body"]),
    ]

    slr_table_data = [
        ["Situación", "Acción"],
        ["El ítem es A → α • a β (a es terminal)", "SHIFT: pasar al estado goto(s, a)"],
        ["El ítem es A → α • (punto al final), a ∈ FOLLOW(A)", "REDUCE: aplicar A → α"],
        ["El ítem es S' → S • (gramática aumentada)", "ACCEPT"],
        ["goto(s, A) existe para no-terminal A", "GOTO: transición al nuevo estado"],
    ]

    slr_t = Table(slr_table_data, colWidths=[7*cm, 9*cm])
    slr_t.setStyle(TableStyle([
        ("BACKGROUND", (0,0), (-1,0), YELLOW),
        ("TEXTCOLOR",  (0,0), (-1,0), BLACK),
        ("FONTNAME",   (0,0), (-1,0), "Helvetica-Bold"),
        ("FONTSIZE",   (0,0), (-1,-1), 9),
        ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.HexColor("#fffbf0"), WHITE]),
        ("GRID",       (0,0), (-1,-1), 0.5, colors.HexColor("#cccccc")),
        ("TOPPADDING",    (0,0), (-1,-1), 5),
        ("BOTTOMPADDING", (0,0), (-1,-1), 5),
        ("LEFTPADDING",   (0,0), (-1,-1), 8),
        ("VALIGN",        (0,0), (-1,-1), "TOP"),
    ]))
    story.append(slr_t)
    story.append(Spacer(1, 0.4*cm))

    # 4.6 LALR
    story += [
        Paragraph("4.6 Tabla LALR(1)", S["h2"]),
        Paragraph(
            "La tabla <b>LALR(1)</b> (Look-Ahead LR) es más potente que SLR. "
            "La diferencia clave es que usa <b>lookaheads específicos por ítem</b> "
            "en vez del FOLLOW completo del no-terminal. Esto reduce los conflictos "
            "shift/reduce en gramáticas donde SLR sería ambigua.",
            S["body"]),
        Paragraph(
            "El proceso: se construye el autómata LR(0) normal, y luego se propagan "
            "los lookaheads desde el estado inicial hasta todos los ítems, "
            "usando la operación de closure con lookaheads (ítems LR(1)).",
            S["body"]),
        Spacer(1, 0.3*cm),
    ]

    # 4.7 LL1
    story += [
        Paragraph("4.7 Tabla LL(1)", S["h2"]),
        Paragraph(
            "La tabla <b>LL(1)</b> es para parsing <b>top-down</b> (de izquierda a derecha, "
            "derivando el símbolo más a la izquierda). Para cada par (no-terminal, terminal), "
            "dice qué producción usar:",
            S["body"]),
        Paragraph(
            "Si A → α y a ∈ FIRST(α): añadir A → α en la celda [A, a].", S["bullet"]),
        Paragraph(
            "Si A → α y ε ∈ FIRST(α) y a ∈ FOLLOW(A): añadir A → α en [A, a].", S["bullet"]),
        Spacer(1, 0.3*cm),
    ]

    # 4.8 Parser
    story += [
        Paragraph("4.8 Parser — Análisis Bottom-Up", S["h2"]),
        Paragraph(
            "El <b>parser</b> en <code>parser.cpp</code> implementa el algoritmo de "
            "parsing <b>shift-reduce</b>. Usa una pila y la tabla de acción+goto:",
            S["body"]),
        Paragraph("<b>SHIFT</b>: empuja el token actual en la pila y avanza al siguiente estado.", S["bullet"]),
        Paragraph("<b>REDUCE</b>: reemplaza los símbolos en la pila que coinciden con el lado "
                  "derecho de una producción por el no-terminal del lado izquierdo.", S["bullet"]),
        Paragraph("<b>ACCEPT</b>: la entrada es válida y se devuelve el árbol sintáctico.", S["bullet"]),
        Paragraph("<b>ERROR</b>: el token actual no es válido en el estado actual.", S["bullet"]),
        Spacer(1, 0.2*cm),
    ]

    parser_trace = (
        "// Ejemplo de traza de parsing para '3 + 4'\n"
        "// Gramática: E -> E + T | T,  T -> INT\n"
        "\n"
        "// Pila      | Entrada restante  | Acción\n"
        "// [0]       | INT PLUS INT $    | shift(s2)\n"
        "// [0,2]     | PLUS INT $        | reduce T->INT\n"
        "// [0,T]     | PLUS INT $        | goto(s4)\n"
        "// [0,4]     | PLUS INT $        | reduce E->T\n"
        "// [0,E]     | PLUS INT $        | goto(s1)\n"
        "// [0,1]     | INT $             | shift(s6)\n"
        "// [0,1,6]   | $                 | reduce T->INT\n"
        "// [0,1,T]   | $                 | goto(s7)\n"
        "// [0,1,7]   | $                 | reduce E->E+T\n"
        "// [0,E]     | $                 | goto(s1)\n"
        "// [0,1]     | $                 | ACCEPT"
    )
    story += code_block("Traza de parsing shift-reduce", parser_trace)
    story.append(Spacer(1, 0.3*cm))

    # 4.9 Conflictos
    story += [
        Paragraph("4.9 Resolución de Conflictos", S["h2"]),
        Paragraph(
            "En algunas gramáticas, la tabla puede tener <b>conflictos</b>: "
            "la misma celda tiene más de una acción posible.",
            S["body"]),
        Paragraph("<b>Shift/Reduce</b>: ¿hacer shift o reducir? Ocurre típicamente con "
                  "la gramática del if-then-else (dangling else).", S["bullet"]),
        Paragraph("<b>Reduce/Reduce</b>: dos producciones distintas pueden reducir. "
                  "Suele indicar ambigüedad real en la gramática.", S["bullet"]),
        Spacer(1, 0.2*cm),
        Paragraph(
            "El módulo <code>conflict_resolver.cpp</code> resuelve automáticamente "
            "los conflictos shift/reduce usando las declaraciones de "
            "<b>precedencia y asociatividad</b> (<code>%left</code>, <code>%right</code>). "
            "Si el token tiene mayor precedencia que la producción, se hace shift; "
            "si la producción tiene mayor precedencia, se reduce.",
            S["body"]),
        PageBreak(),
    ]

    # ════════════════════════════════════════════════════════
    # 5. API
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("5. API REST (Python / FastAPI)", S["h1"]),
        section_rule(MAUVE),
        Paragraph(
            "La API es el puente entre el frontend y el núcleo C++. "
            "Está construida con <b>FastAPI</b>, que genera automáticamente "
            "documentación interactiva en <code>/docs</code>.",
            S["body"]),
        Spacer(1, 0.2*cm),
        Paragraph("Endpoints principales:", S["h3"]),
    ]

    endpoints_data = [
        ["Endpoint", "Método", "Descripción"],
        ["/yalex/analyze", "POST", "Analiza un .yalex + texto → tokens, DFA, DOT graph"],
        ["/yapar/analyze", "POST", "Analiza un .yapar → gramática, autómata LR(0), tablas SLR/LALR/LL1, árbol"],
        ["/natural/translate", "POST", "Traduce texto en Q'eqchi' u otros lenguajes naturales"],
        ["/graphviz/*", "GET", "Genera imágenes SVG/PNG desde código DOT (Graphviz)"],
    ]

    ep_t = Table(endpoints_data, colWidths=[5*cm, 2.5*cm, 8.5*cm])
    ep_t.setStyle(TableStyle([
        ("BACKGROUND", (0,0), (-1,0), MAUVE),
        ("TEXTCOLOR",  (0,0), (-1,0), BLACK),
        ("FONTNAME",   (0,0), (-1,0), "Helvetica-Bold"),
        ("FONTSIZE",   (0,0), (-1,-1), 9),
        ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.HexColor("#faf0ff"), WHITE]),
        ("GRID",       (0,0), (-1,-1), 0.5, colors.HexColor("#cccccc")),
        ("TOPPADDING",    (0,0), (-1,-1), 5),
        ("BOTTOMPADDING", (0,0), (-1,-1), 5),
        ("LEFTPADDING",   (0,0), (-1,-1), 8),
        ("VALIGN",        (0,0), (-1,-1), "TOP"),
    ]))
    story.append(ep_t)
    story.append(Spacer(1, 0.5*cm))

    api_code = (
        "# routers/yalex.py — Endpoint /yalex/analyze\n"
        "@router.post('/analyze')\n"
        "def analyze(req: YAlexRequest):\n"
        "    # 1. Escribir archivos temporales (evita problemas con\n"
        "    #    caracteres especiales en argumentos shell)\n"
        "    with tempfile.NamedTemporaryFile(suffix='.yalex') as f:\n"
        "        f.write(req.yalex_content)\n"
        "        yalex_path = f.name\n"
        "\n"
        "    # 2. Ejecutar el binario C++ compilado\n"
        "    result = subprocess.run(\n"
        "        [YALEX_CLI, yalex_path, input_path],\n"
        "        capture_output=True, text=True, timeout=30\n"
        "    )\n"
        "\n"
        "    # 3. Parsear el JSON que devuelve el binario y retornarlo\n"
        "    return json.loads(result.stdout)"
    )
    story += code_block("Lógica de yalex.py (simplificada)", api_code)

    story += [
        Paragraph(
            "El patrón es siempre el mismo: escribir en archivos temporales → "
            "ejecutar binario C++ → parsear JSON de stdout → devolver al frontend. "
            "Los archivos temporales se eliminan siempre en el bloque <code>finally</code>.",
            S["body"]),
        PageBreak(),
    ]

    # ════════════════════════════════════════════════════════
    # 6. FRONTEND
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("6. Frontend (React / TypeScript)", S["h1"]),
        section_rule(TEAL),
        Paragraph(
            "El frontend es un <b>IDE web completo</b> construido con React 18 y TypeScript. "
            "El archivo principal <code>App.tsx</code> tiene ~938 líneas y contiene toda "
            "la lógica del IDE.",
            S["body"]),
        Spacer(1, 0.2*cm),
        Paragraph("Componentes principales:", S["h3"]),
    ]

    components = [
        ["Componente", "Archivo", "Función"],
        ["IDE Principal", "App.tsx", "Coordina editores, estado global, llamadas a la API"],
        ["Editor de Código", "Editor/CodeEditor.tsx", "Monaco Editor con syntax highlighting y marcadores de error"],
        ["Vista de Autómata", "Automaton/AutomatonView.tsx", "Grafo del autómata LR(0) con Graphviz"],
        ["Tablas de Parsing", "Tables/TablesView.tsx", "Tablas SLR/LALR/LL1 con celdas coloreadas por tipo de acción"],
        ["FIRST/FOLLOW", "FirstFollow/FirstFollowView.tsx", "Conjuntos FIRST y FOLLOW tabulados"],
        ["Desambiguación", "Disambiguation/DisambiguationView.tsx", "Detalle de conflictos shift/reduce y su resolución"],
        ["Árbol Sintáctico", "SyntaxTree/TreeView.tsx", "Parse tree visual interactivo"],
        ["Consola", "Console/ConsolePanel.tsx", "Log de errores y mensajes, clickable para navegar al error"],
    ]

    comp_t = Table(components, colWidths=[4*cm, 5*cm, 7*cm])
    comp_t.setStyle(TableStyle([
        ("BACKGROUND", (0,0), (-1,0), TEAL),
        ("TEXTCOLOR",  (0,0), (-1,0), BLACK),
        ("FONTNAME",   (0,0), (-1,0), "Helvetica-Bold"),
        ("FONTSIZE",   (0,0), (-1,-1), 8.5),
        ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.HexColor("#f0fffe"), WHITE]),
        ("GRID",       (0,0), (-1,-1), 0.5, colors.HexColor("#cccccc")),
        ("TOPPADDING",    (0,0), (-1,-1), 4),
        ("BOTTOMPADDING", (0,0), (-1,-1), 4),
        ("LEFTPADDING",   (0,0), (-1,-1), 6),
        ("VALIGN",        (0,0), (-1,-1), "TOP"),
    ]))
    story.append(comp_t)
    story.append(Spacer(1, 0.4*cm))

    story += [
        Paragraph("Características destacadas:", S["h3"]),
        Paragraph("<b>Detección automática de lenguaje:</b> el IDE detecta si estás "
                  "editando .yalex, .yapar, Q'eqchi', MessiScript, etc., y cambia "
                  "el tema de colores automáticamente.", S["bullet"]),
        Paragraph("<b>Marcadores de error inline:</b> los errores del backend se muestran "
                  "como subrayados rojos en el editor Monaco (igual que VS Code). "
                  "Hacer clic en el error en la consola salta a la línea exacta.", S["bullet"]),
        Paragraph("<b>Estado global con Zustand:</b> todo el estado (archivos, resultados, "
                  "errores, tema) vive en un store global para evitar prop-drilling.", S["bullet"]),
        Paragraph("<b>Tres editores:</b> uno para el .yalex, uno para el .yapar, y uno "
                  "para el texto de entrada a analizar.", S["bullet"]),
        Paragraph("<b>Seis pestañas de visualización:</b> Tokens, Autómata, Tablas, "
                  "FIRST/FOLLOW, Desambiguación, Traducción.", S["bullet"]),
        PageBreak(),
    ]

    # ════════════════════════════════════════════════════════
    # 7. FLUJO COMPLETO
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("7. Flujo Completo — Ejemplo Paso a Paso", S["h1"]),
        section_rule(RED),
        Paragraph(
            "Imaginemos que queremos analizar la expresión <code>3 + 4 * 2</code> "
            "con una gramática de expresiones aritméticas. Esto es lo que ocurre "
            "internamente cuando el usuario hace clic en \"Ejecutar\":",
            S["body"]),
        Spacer(1, 0.2*cm),
    ]

    steps = [
        ("Paso 1 — Frontend envía la petición",
         "El navegador hace POST /yalex/analyze con el contenido del .yalex y el texto '3 + 4 * 2'."),
        ("Paso 2 — API recibe y prepara",
         "FastAPI recibe el JSON, escribe el .yalex en /tmp/abc.yalex y el texto en /tmp/def.txt."),
        ("Paso 3 — Ejecuta yalex_cli",
         "Invoca: ./core/build/yalex_cli /tmp/abc.yalex /tmp/def.txt\n"
         "El binario lee el .yalex, construye DFA, y tokeniza el texto."),
        ("Paso 4 — Tokens reconocidos",
         "DFA reconoce: INT(3), PLUS(+), INT(4), TIMES(*), INT(2), EOF($)\n"
         "El binario imprime en stdout un JSON con los tokens y el DFA."),
        ("Paso 5 — API reenvía al frontend",
         "FastAPI parsea el JSON y lo retorna. El frontend actualiza la pestaña 'Tokens'."),
        ("Paso 6 — Análisis sintáctico",
         "Frontend envía POST /yapar/analyze con el .yapar y los tokens.\n"
         "yapar_cli carga la gramática, computa FIRST/FOLLOW, construye el LR(0), genera tablas."),
        ("Paso 7 — Parsing shift-reduce",
         "El parser procesa: SHIFT(3), REDUCE(T->INT), SHIFT(+), SHIFT(4), REDUCE(T->INT),\n"
         "SHIFT(*), SHIFT(2), REDUCE(T->T*INT), REDUCE(E->E+T), ACCEPT.\n"
         "Produce el árbol sintáctico."),
        ("Paso 8 — Visualizaciones",
         "El frontend recibe el JSON y actualiza: autómata LR(0), tablas SLR/LALR, "
         "conjuntos FIRST/FOLLOW, árbol sintáctico. El usuario puede explorar cada vista."),
    ]

    for title, desc in steps:
        story += [
            KeepTogether([
                Paragraph(f"<b>{title}</b>", S["h3"]),
                Paragraph(desc.replace("\n", "<br/>"), S["body"]),
                Spacer(1, 0.15*cm),
            ])
        ]

    story.append(PageBreak())

    # ════════════════════════════════════════════════════════
    # 8. FORMATOS DE ENTRADA
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("8. Formatos de Entrada: .yalex y .yapar", S["h1"]),
        section_rule(GREEN),
        Paragraph("Guía de referencia rápida para escribir los archivos de especificación.", S["body"]),
        Spacer(1, 0.2*cm),
        Paragraph("Formato .yalex completo:", S["h2"]),
    ]

    yalex_full = (
        "(* Comentarios con (* ... *) o con //  *)\n"
        "\n"
        "(* 1. Definición de macros (let)            *)\n"
        "let digit  = ['0'-'9']\n"
        "let upper  = ['A'-'Z']\n"
        "let lower  = ['a'-'z']\n"
        "let letter = upper | lower\n"
        "let id     = letter (letter | digit | '_')*\n"
        "let ws     = ' ' | '\\t' | '\\n'\n"
        "\n"
        "(* 2. Reglas léxicas (rule tokens = ...)    *)\n"
        "rule tokens =\n"
        "  digit+           { INT    }\n"
        "| id               { ID     }\n"
        "| '+'              { PLUS   }\n"
        "| '-'              { MINUS  }\n"
        "| '*'              { TIMES  }\n"
        "| '/'              { DIV    }\n"
        "| '('              { LPAREN }\n"
        "| ')'              { RPAREN }\n"
        "| ws               { ignore }\n"
    )
    story += code_block("Plantilla de archivo .yalex", yalex_full)
    story.append(Spacer(1, 0.3*cm))

    story.append(Paragraph("Formato .yapar completo:", S["h2"]))

    yapar_full = (
        "(* Sección de declaraciones *)\n"
        "%token INT ID PLUS MINUS TIMES DIV LPAREN RPAREN\n"
        "%start program\n"
        "\n"
        "(* Precedencia: más abajo = más alta precedencia *)\n"
        "%left  PLUS MINUS\n"
        "%left  TIMES DIV\n"
        "\n"
        "(* Separador OBLIGATORIO *)\n"
        "%%\n"
        "\n"
        "(* Sección de reglas gramaticales *)\n"
        "program : expr\n"
        "        ;\n"
        "\n"
        "expr : expr PLUS  term  (* suma        *)\n"
        "     | expr MINUS term  (* resta       *)\n"
        "     | term\n"
        "     ;\n"
        "\n"
        "term : term TIMES factor\n"
        "     | term DIV   factor\n"
        "     | factor\n"
        "     ;\n"
        "\n"
        "factor : LPAREN expr RPAREN\n"
        "       | INT\n"
        "       | ID\n"
        "       ;"
    )
    story += code_block("Plantilla de archivo .yapar", yapar_full)

    story += [
        Paragraph(
            "<b>Nota importante:</b> los terminales deben escribirse en MAYÚSCULAS "
            "y los no-terminales en minúsculas. El separador <code>%%</code> es "
            "obligatorio.",
            S["note"]),
        PageBreak(),
    ]

    # ════════════════════════════════════════════════════════
    # 9. CONCLUSIONES
    # ════════════════════════════════════════════════════════
    story += [
        Paragraph("9. Resumen y Conclusiones", S["h1"]),
        section_rule(ACCENT),
        Paragraph(
            "Este proyecto implementa desde cero una <b>cadena completa de compilación</b>: "
            "desde leer una especificación léxica en texto plano hasta producir un árbol "
            "sintáctico visualizable en el navegador.",
            S["body"]),
        Spacer(1, 0.3*cm),
        Paragraph("Lo que se implementó:", S["h2"]),
    ]

    summary_items = [
        ("Análisis Léxico (YALex)",
         "Parsing de .yalex → Expansión de macros y clases de caracteres → "
         "Conversión de regex a postfix (Shunting-Yard) → NFA (Thompson) → "
         "DFA (Subconjuntos) → DFA mínimo (Hopcroft) → Tokenizador."),
        ("Análisis Sintáctico (YAPar)",
         "Parsing de .yapar → Gramática aumentada → FIRST/FOLLOW (punto fijo) → "
         "Autómata LR(0) (closure + goto) → Tabla SLR(1) → Tabla LALR(1) → "
         "Tabla LL(1) → Parser shift-reduce → Árbol sintáctico."),
        ("Resolución de conflictos",
         "Detección de conflictos shift/reduce y reduce/reduce. Resolución automática "
         "usando precedencia y asociatividad declaradas con %left/%right/%nonassoc."),
        ("API REST",
         "FastAPI con endpoints para YALex, YAPar, traducción natural y Graphviz. "
         "Patrón: archivos temporales → subprocess → JSON → frontend."),
        ("Frontend IDE",
         "Monaco Editor con marcadores inline, 6 pestañas de visualización, "
         "detección automática de lenguaje, temas por lenguaje, consola estructurada."),
    ]

    for title, desc in summary_items:
        story += [
            Paragraph(f"<b>{title}</b>", S["h3"]),
            Paragraph(desc, S["body"]),
            Spacer(1, 0.1*cm),
        ]

    story += [
        Spacer(1, 0.3*cm),
        Paragraph("Tecnologías utilizadas:", S["h2"]),
    ]

    tech_data = [
        ["Capa", "Tecnología", "Versión / Detalles"],
        ["Core C++", "C++17 + CMake 3.20+", "nlohmann/json para I/O, sin librerías de parsing externas"],
        ["API", "Python 3 + FastAPI", "Pydantic, CORS, subprocesos, archivos temporales"],
        ["Frontend", "React 18 + TypeScript", "Vite, Monaco Editor, Framer Motion, Zustand, D3.js"],
        ["Build", "CMake + Ninja", "Compilación en /core/build, produce yalex_cli y yapar_cli"],
    ]

    tech_t = Table(tech_data, colWidths=[4*cm, 5*cm, 7*cm])
    tech_t.setStyle(TableStyle([
        ("BACKGROUND", (0,0), (-1,0), ACCENT),
        ("TEXTCOLOR",  (0,0), (-1,0), WHITE),
        ("FONTNAME",   (0,0), (-1,0), "Helvetica-Bold"),
        ("FONTSIZE",   (0,0), (-1,-1), 9),
        ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.HexColor("#f0f4ff"), WHITE]),
        ("GRID",       (0,0), (-1,-1), 0.5, colors.HexColor("#cccccc")),
        ("TOPPADDING",    (0,0), (-1,-1), 5),
        ("BOTTOMPADDING", (0,0), (-1,-1), 5),
        ("LEFTPADDING",   (0,0), (-1,-1), 8),
        ("VALIGN",        (0,0), (-1,-1), "TOP"),
    ]))
    story.append(tech_t)
    story.append(Spacer(1, 0.5*cm))

    story += [
        section_rule(ACCENT),
        Spacer(1, 0.2*cm),
        Paragraph(
            "El proyecto demuestra que es posible implementar un compilador web completo "
            "combinando C++ para el rendimiento del núcleo algorítmico, Python para la "
            "integración rápida vía API, y React para una experiencia de usuario rica. "
            "Cada algoritmo (Thompson, Subconjuntos, Hopcroft, LR(0), FIRST/FOLLOW, "
            "SLR, LALR) está implementado desde cero en C++17 sin librerías externas "
            "de parsing.",
            S["body"]),
    ]

    # Build
    doc.build(story)
    print(f"PDF generado: {OUTPUT}")

if __name__ == "__main__":
    build()
