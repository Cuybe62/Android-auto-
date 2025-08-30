import { writable } from 'svelte/store';export const modules=writable<Record<string,any>>({});export const status=writable('init…');
