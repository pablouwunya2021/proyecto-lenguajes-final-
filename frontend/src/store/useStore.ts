import { create } from 'zustand';
import type { ActiveModule, ActiveView, YAlexResult, YAParResult } from '../types';
import type { ConsoleEntry } from '../types/console';

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

let _entryCounter = 0;
export const makeId = () => `e${Date.now()}-${_entryCounter++}`;

interface IDEState {
  activeModule:    ActiveModule;
  activeView:      ActiveView;
  yalexContent:    string;
  yaparContent:    string;
  inputText:       string;
  yalexResult:     YAlexResult | null;
  yaparResult:     YAParResult | null;
  isLoading:       boolean;
  globalError:     string | null;
  consoleEntries:  ConsoleEntry[];
  consoleOpen:     boolean;

  setActiveModule:   (m: ActiveModule) => void;
  setActiveView:     (v: ActiveView)   => void;
  setYalexContent:   (c: string)       => void;
  setYaparContent:   (c: string)       => void;
  setInputText:      (t: string)       => void;
  setYalexResult:    (r: YAlexResult | null) => void;
  setYaparResult:    (r: YAParResult | null) => void;
  setLoading:        (l: boolean)      => void;
  setGlobalError:    (e: string | null) => void;
  addConsoleEntry:   (e: Omit<ConsoleEntry, 'id' | 'timestamp'>) => void;
  addConsoleEntries: (es: Omit<ConsoleEntry, 'id' | 'timestamp'>[]) => void;
  clearConsole:      () => void;
  setConsoleOpen:    (open: boolean) => void;

  resetResults: () => void;
}

export const useStore = create<IDEState>((set) => ({
  activeModule:   'yalex',
  activeView:     'editor',
  yalexContent:   EJEMPLO_YALEX,
  yaparContent:   EJEMPLO_YAPAR,
  inputText:      'x + 42;',
  yalexResult:    null,
  yaparResult:    null,
  isLoading:      false,
  globalError:    null,
  consoleEntries: [],
  consoleOpen:    true,

  setActiveModule:  (m) => set({ activeModule: m }),
  setActiveView:    (v) => set({ activeView: v }),
  setYalexContent:  (c) => set({ yalexContent: c }),
  setYaparContent:  (c) => set({ yaparContent: c }),
  setInputText:     (t) => set({ inputText: t }),
  setYalexResult:   (r) => set({ yalexResult: r }),
  setYaparResult:   (r) => set({ yaparResult: r }),
  setLoading:       (l) => set({ isLoading: l }),
  setGlobalError:   (e) => set({ globalError: e }),
  setConsoleOpen:   (open) => set({ consoleOpen: open }),

  addConsoleEntry: (e) => set((s) => ({
    consoleEntries: [...s.consoleEntries, { ...e, id: makeId(), timestamp: Date.now() }],
    consoleOpen: true,
  })),

  addConsoleEntries: (es) => set((s) => ({
    consoleEntries: [
      ...s.consoleEntries,
      ...es.map(e => ({ ...e, id: makeId(), timestamp: Date.now() })),
    ],
    consoleOpen: true,
  })),

  clearConsole: () => set({ consoleEntries: [] }),

  resetResults: () => set({
    yalexResult:  null,
    yaparResult:  null,
    globalError:  null,
    isLoading:    false,
  }),
}));
