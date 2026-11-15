import { Routes, Route, Link, useLocation } from 'react-router-dom';
import { useEffect } from 'react';
import AppShell, { RequireAuth, HomeRoute } from './components/AppShell.jsx';
import Landing from './pages/Landing.jsx';
import Auth from './pages/Auth.jsx';
import Dashboard from './pages/Dashboard.jsx';
import Search from './pages/Search.jsx';
import { MyProfile, StudentProfile } from './pages/Profile.jsx';
import Review from './pages/Review.jsx';
import { Empty, BrandMark } from './components/ui.jsx';

function ScrollToTop() {
  const { pathname } = useLocation();
  useEffect(() => {
    window.scrollTo(0, 0);
  }, [pathname]);
  return null;
}

function NotFound() {
  return (
    <div className="notfound">
      <BrandMark />
      <Empty icon="search" title="This page does not exist" action={<Link to="/" className="btn btn-primary">Go to the home page</Link>}>
        The link may be old or mistyped.
      </Empty>
    </div>
  );
}

export default function App() {
  return (
    <>
      <ScrollToTop />
      <Routes>
        <Route path="/" element={<Landing />} />
        <Route path="/login" element={<Auth mode="login" />} />
        <Route path="/signup" element={<Auth mode="signup" />} />
        <Route
          path="/app"
          element={
            <RequireAuth>
              <AppShell />
            </RequireAuth>
          }
        >
          <Route index element={<HomeRoute><Dashboard /></HomeRoute>} />
          <Route path="search" element={<Search />} />
          <Route path="profile" element={<RequireAuth role="student"><MyProfile /></RequireAuth>} />
          <Route path="review" element={<RequireAuth role="admin"><Review /></RequireAuth>} />
          <Route path="students/:id" element={<StudentProfile />} />
        </Route>
        <Route path="*" element={<NotFound />} />
      </Routes>
    </>
  );
}
