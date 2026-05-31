export interface Token { type: string; lexeme: string; line: number; col: number; }
export interface LexError { message: string; line: number; col: number; }
export interface DFAState { id: number; accepting: boolean; token: string; }
export interface DFATransition { from: number; to: number; label: string; }
export interface DFA { states: DFAState[]; transitions: DFATransition[]; start: number; }
export interface YAlexResult { success: boolean; tokens: Token[]; errors: LexError[]; dfa: DFA; }
export interface Production { id: number; lhs: string; rhs: string[]; }
export interface LR0State { id: number; accepting: boolean; items: string[]; }
export interface LR0Transition { from: number; to: number; label: string; }
export interface LR0Automaton { states: LR0State[]; transitions: LR0Transition[]; }
export interface TableEntry { state: number; symbol: string; action: string; }
export interface GotoEntry { state: number; symbol: string; goto: number; }
export interface ParseNode { name: string; lexeme?: string; children?: ParseNode[]; }
export interface ConflictAnalysis { description: string; reason: string; shiftDepth: number; reduceLength: number; }
export interface YAParResult {
  success: boolean;
  grammar: { productions: Production[]; start: string };
  automaton: LR0Automaton;
  slr: { action: TableEntry[]; goto: GotoEntry[]; conflicts: any[] };
  ll1: { table: any[] };
  lalr: any;
  conflicts: ConflictAnalysis[];
  tree?: ParseNode;
  errors?: string[];
}
export type ActiveModule = 'yalex' | 'yapar' | 'maya' | 'cow' | 'messi';
export type ActiveView   = 'editor' | 'automaton' | 'tables' | 'tree' | 'parallel';
