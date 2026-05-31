import { create } from 'zustand';
import type { ActiveModule, ActiveView, YAlexResult, YAParResult } from '../types';

const EJEMPLO_YALEX = `(* Especificacion YALex *)
{
  let letter = [a-zA-Z_]
  let digit  = [0-9]
  let alnum  = [a-zA-Z0-9_]
  let ws     = [' ' '\\t' '\\n']
}
rule tokens =
    {letter}{alnum}*   { ID }
  | {digit}+           { INT }
  | {ws}+              { WHITESPACE }
  | ">="               { GEQ }
  | "="                { ASSIGN }
  | "+"                { PLUS }
  | "-"                { MINUS }
  | "*"                { TIMES }
  | "/"                { DIV }
  | "("                { LPAREN }
  | ")"                { RPAREN }
  | ";"                { SEMICOLON }`;

const EJEMPLO_YAPAR = `%token INT FLOAT ID PLUS MINUS TIMES DIV LPAREN RPAREN SEMICOLON
%start program
%%
program : stmt_list
stmt_list : stmt_list stmt
          | stmt
stmt : expr SEMICOLON
expr : expr PLUS  term
     | expr MINUS term
     | term
term : term TIMES factor
     | term DIV   factor
     | factor
factor : LPAREN expr RPAREN
       | INT
       | FLOAT
       | ID`;

interface IDEState {
  activeModule: ActiveModule; setActiveModule: (m: ActiveModule) => void;
  activeView:   ActiveView;   setActiveView:   (v: ActiveView) => void;
  yalexContent: string; setYalexContent: (c: string) => void;
  yaparContent: string; setYaparContent: (c: string) => void;
  inputText:    string; setInputText:    (t: string) => void;
  yalexResult:  YAlexResult | null; setYalexResult: (r: YAlexResult | null) => void;
  yaparResult:  YAParResult | null; setYaparResult: (r: YAParResult | null) => void;
  isLoading:  boolean; setLoading:    (l: boolean) => void;
  globalError: string | null; setGlobalError: (e: string | null) => void;
}

export const useStore = create<IDEState>((set) => ({
  activeModule: 'yalex', setActiveModule: (m) => set({ activeModule: m, yalexResult: null, yaparResult: null }),
  activeView:   'editor', setActiveView:   (v) => set({ activeView: v }),
  yalexContent: EJEMPLO_YALEX, setYalexContent: (c) => set({ yalexContent: c }),
  yaparContent: EJEMPLO_YAPAR, setYaparContent: (c) => set({ yaparContent: c }),
  inputText:    'x + 42;',     setInputText:    (t) => set({ inputText: t }),
  yalexResult:  null, setYalexResult: (r) => set({ yalexResult: r }),
  yaparResult:  null, setYaparResult: (r) => set({ yaparResult: r }),
  isLoading:    false, setLoading:    (l) => set({ isLoading: l }),
  globalError:  null,  setGlobalError: (e) => set({ globalError: e }),
}));
