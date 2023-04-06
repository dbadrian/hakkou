import { createApp } from "vue";
// import { createApp } from 'vue/dist/vue.esm-bundler';
import App from "./App.vue";
import router from "./router";
import "./assets/main.css";

const app = createApp(App);


app.use(router);

app.mount("#app");
