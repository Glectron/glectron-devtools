// This is a minimum polyfill used by Awesomium DevTools
// Currently it still doesn't cover all deprecated APIs used by Awesomium DevTools

class CSSPrimitiveValue {
    CSS_PX = 18
}

class CSSValue {
    value
    constructor(value) {
        this.value = value;
    }

    getFloatValue(primitiveType) {
        const regex = /[+-]?\d+(\.\d+)?/g;
        return this.value.match(regex).map(function(v) { return parseFloat(v); });
    }
}

CSSStyleDeclaration.prototype.getPropertyCSSValue = function(name) {
    return new CSSValue(this.getPropertyValue(name));
}

Object.defineProperty(KeyboardEvent.prototype, "keyIdentifier", {
    get() {
        const valueMaps = {
            "ArrowUp": "Up",
            "ArrowDown": "Down",
            "ArrowLeft": "Left",
            "ArrowRight": "Right",
            "Tab": "U+0009",
            "Escape": "U+001B",
            " ": "U+0020"
        };
        return valueMaps[this.key] || this.key;
    }
})