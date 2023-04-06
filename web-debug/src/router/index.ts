import { createRouter, createWebHistory } from "vue-router";
// import HomeView from "../views/HomeView.vue";
import PageNotFound from "../views/PageNotFound.vue";

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: "/",
      name: "home",
      component: () => import("../views/GUIControl.vue"),
    },
    // {
    //   path: "/comics",
    //   name: "comics",
    //   component: () => import("../views/ComicView.vue"),
    // },
    // {
    //   path: "/comics/:id",
    //   name: "comics-by-id",
    //   component: () => import("../views/ComicView.vue"),
    // },
    // {
    //   path: "/about",
    //   name: "about",
    //   // route level code-splitting
    //   // this generates a separate chunk (About.[hash].js) for this route
    //   // which is lazy-loaded when the route is visited.
    //   component: () => import("../views/AboutView.vue"),
    // },
    // {
    //   path: "/shop",
    //   name: "aboushop",
    //   // route level code-splitting
    //   // this generates a separate chunk (About.[hash].js) for this route
    //   // which is lazy-loaded when the route is visited.
    //   component: () => import("../views/ShopView.vue"),
    // },
    { path: '/:pathMatch(.*)*', component: PageNotFound }
  ],
});

export default router;
