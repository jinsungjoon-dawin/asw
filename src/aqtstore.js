import { readable, writable } from 'svelte/store';
import ko from './locales/ko.json';

let value = "";
let aqtpass = 'abcd';

export const gtcode = writable(value);
export const isLogged = writable(0);
export const authApps = writable("");
export const userid = writable("");
export const appids = writable([]);
// export const rooturl = readable('http://localhost:5880');
export const rooturl = readable('http://192.168.0.10:5555');
export const intlMs = writable(0);
export const t = readable(ko);
export const getCheckPass = async (password) => {
	if (password === aqtpass)
		return 1
};

export const setCheckPass = async (password) => {
	aqtpass = password;
}